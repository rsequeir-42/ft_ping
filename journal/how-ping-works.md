# Un écho dans le réseau

Avant d'écrire la première ligne de réseau, je voulais comprendre précisément ce que fait la commande que je m'apprête à recoder. `ping` a l'air trivial — on tape une adresse, une réponse tombe, on en conclut que « ça marche ». Mais sous cette surface se cache une mécanique exacte, faite d'en-têtes au bit près, d'une arithmétique de vérification et d'une prise réseau hors du commun. La reproduire fidèlement suppose de la connaître dans le détail : c'est l'objet de ce détour.

## ICMP : la voie de service du réseau

Une machine qui communique empile des protocoles comme des poupées russes. Tout en bas, **IP** (*Internet Protocol*) sait acheminer un bloc d'octets d'une adresse à une autre, à travers les routeurs — sans garantie, ni d'arrivée, ni d'ordre. Au-dessus, on trouve d'ordinaire **TCP** (le Web, les fichiers) ou **UDP** (la voix, le jeu), qui ajoutent la notion de *port* pour distinguer les multiples conversations d'une même machine.

`ping`, lui, parle **ICMP** (*Internet Control Message Protocol*). C'est un protocole à part : il ne transporte pas de données applicatives mais des **messages de contrôle** — « destination injoignable », « durée de vie expirée », « me voici en écho ». ICMP n'a pas de port ; il s'appuie directement sur IP, qui le repère par son **numéro de protocole, 1** (TCP est 6, UDP 17). Concrètement, un message ICMP est *encapsulé* dans un paquet IP : l'en-tête IP (20 octets) d'abord, le message ICMP ensuite.

Chaque message ICMP porte d'abord un **type** sur un octet. La table est longue (type 3 « destination unreachable », type 11 « time exceeded »…), mais `ping` n'en utilise qu'une poignée. Le cœur est le couple **Echo Request** (type 8) / **Echo Reply** (type 0) : j'envoie un type 8, une machine vivante me répond par un type 0 dont le contenu recopie le mien. L'écho, littéralement.

## L'en-tête ICMP, octet par octet

Le message Echo commence par un en-tête de **8 octets**, dont voici la disposition (les chiffres du haut sont les positions de bits) :

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+---------------+---------------+-------------------------------+
|     Type      |     Code      |           Checksum            |
+---------------+---------------+-------------------------------+
|         Identifier            |        Sequence Number        |
+-------------------------------+-------------------------------+
|     Données (payload, 56 octets par défaut) ...
+-----------------------------------------------------
```

Sur Linux, cet en-tête correspond à une structure C bien réelle, `struct icmphdr` (dans `<netinet/ip_icmp.h>`) :

```c
struct icmphdr {
    uint8_t  type;        /* 8 = Echo Request, 0 = Echo Reply */
    uint8_t  code;        /* 0 pour un écho */
    uint16_t checksum;    /* somme de contrôle (voir plus bas) */
    union {
        struct { uint16_t id; uint16_t sequence; } echo;
        uint32_t gateway; /* autres types ICMP : autre interprétation */
        /* ... */
    } un;
};
```

Deux remarques. D'abord, l'`union` : ces 4 derniers octets changent de sens selon le type de message — pour un écho, ce sont l'**identifiant** et la **séquence** ; pour une redirection, une adresse de passerelle ; etc. Un même emplacement, plusieurs lectures. Ensuite, les champs de 16 et 32 bits voyagent en **ordre réseau** (*big-endian*, octet de poids fort d'abord), qui n'est pas forcément celui du processeur : il faudra convertir avec `htons`/`ntohs` à l'écriture et à la lecture.

## Le sceau du checksum

Le champ `checksum` est un **sceau de cohérence** : il ne chiffre rien, il permet seulement au récepteur de détecter qu'un bit a été altéré en route. ICMP, IP, TCP et UDP utilisent tous le même « *Internet checksum* » de la RFC 1071. Son principe mérite qu'on s'y arrête, car il est plus subtil qu'une simple addition.

On regarde le message comme une suite d'entiers de **16 bits**, et on les additionne. Une addition de mots de 16 bits déborde vite : on accumule donc dans un registre de 32 bits pour ne rien perdre. Puis on **replie** les retenues — on rajoute les 16 bits de poids fort aux 16 bits de poids faible — et on répète tant que ça déborde. Ce repli, c'est ce qui fait de l'opération une *addition en complément à un* : au lieu d'être perdues, les retenues « tournent » et reviennent par le bas. Enfin, on prend le **complément à un** du résultat, c'est-à-dire son inversion bit à bit. En C, cela tient en quelques lignes :

```c
uint16_t checksum(const void *data, size_t len) {
    const uint16_t *w = data;
    uint32_t sum = 0;
    while (len > 1) { sum += *w++; len -= 2; }   /* somme des mots de 16 bits */
    if (len) sum += *(const uint8_t *)w;          /* octet final esseulé */
    sum = (sum >> 16) + (sum & 0xffff);           /* repli des retenues */
    sum += (sum >> 16);                           /* la dernière retenue */
    return ~sum;                                  /* complément à un */
}
```

L'élégance est dans la vérification. L'émetteur met d'abord le champ `checksum` à zéro, calcule la somme, et y inscrit son complément. Le récepteur, lui, additionne **tout, checksum compris** : si rien n'a bougé, la somme repliée vaut `0xFFFF` (tous les bits à un), dont le complément est zéro. Autrement dit, un paquet intact se reconnaît à une somme nulle — un seul test, sans avoir à ressortir la valeur d'origine.

Deux détails d'implémentation que le code ci-dessus traîne discrètement. Si la longueur est **impaire**, il reste un octet seul : on le traite comme un mot de 16 bits complété par un octet nul à droite. Et l'on additionne les mots **sans convertir leur ordre d'octets** : comme l'addition en complément à un est commutative et que permuter les octets de la somme revient à permuter les octets des termes, le résultat reste correct dans les deux sens — une optimisation classique qui évite des milliers de `ntohs`.

## Reconnaître ses propres échos

Restent l'**identifiant** et la **séquence**, ces deux champs de 16 bits qui semblent anodins mais portent toute la logique de reconnaissance.

Le problème : un *raw socket* ICMP (j'y viens) reçoit **tous** les échos qui transitent par la machine, pas seulement les nôtres. Si deux `ping` tournent en parallèle, chacun voit les réponses de l'autre. Pour démêler, `ping` inscrit dans l'**identifiant** un nombre qui lui est propre — typiquement son **PID**, le numéro de processus, tronqué à 16 bits (`ident & 0xFFFF`). Toute réponse dont l'identifiant ne correspond pas est écartée d'emblée.

La **séquence**, elle, s'incrémente d'une unité à chaque envoi. Elle a deux usages. Le premier est visible : c'est le `icmp_seq=1, 2, 3…` qui défile, et un trou dans la suite trahit un paquet perdu. Le second est interne : `ping` tient une **table de bits** (un *bitmap*) où il coche chaque séquence reçue. Si le bit était déjà mis, c'est qu'un paquet est revenu en **double** (un routeur l'a dupliqué) ; sinon, c'est une première réception. Cela permet de compter justement, et d'afficher le fameux `(DUP!)` le cas échéant.

## Une charge qui sert à mesurer

Vient la charge utile — 56 octets par défaut, soit les 64 octets totaux affichés moins les 8 de l'en-tête. (Gare au décompte : le paquet réellement émis sur le fil est plus gros encore — 20 octets d'en-tête IP par-dessus, soit **84 octets** en tout —, mais `ping` n'affiche que la partie **ICMP**, en-tête plus données ; l'en-tête IP, posé et retiré par le système, reste hors du « 64 bytes ». C'est une perplexité fréquente quand on observe le trafic à côté.) Elle n'est pas du remplissage gratuit : `ping` y range, **tout au début, un horodatage**. Sur Linux, c'est une `struct timeval` (deux entiers : secondes et microsecondes, 16 octets sur un système 64 bits) obtenue par `gettimeofday()` au moment de l'envoi, et recopiée telle quelle dans les premiers octets du payload. Le reste — ce qui suit l'horodatage — est rempli d'un **motif connu** : par défaut, chaque octet vaut son propre rang (`0x00, 0x01, 0x02, …`), ce qui permet, au retour, de vérifier que les données n'ont pas été corrompues.

Ce timestamp embarqué n'est inséré que si la charge est assez grande pour l'accueillir (au moins la taille d'une `struct timeval`). Avec les 56 octets par défaut, il y tient largement ; mais un `ping -s 4` (4 octets de charge) renoncera au chronométrage, faute de place.

## Mesurer le temps sans rien retenir

C'est ici que l'horodatage embarqué prend tout son sens. Une approche naïve voudrait que `ping` garde en mémoire, pour chaque paquet émis, l'heure de départ, afin de la retrouver au retour. Mais c'est inutile : la cible recopie le payload **à l'identique**, horodatage compris. Le temps de départ revient donc *dans* le paquet. À la réception, `ping` relit l'horloge et soustrait :

```c
/* à l'émission */
gettimeofday(&t0, NULL);
memcpy(payload, &t0, sizeof t0);
/* ... le paquet part, revient ... */
/* à la réception */
gettimeofday(&t1, NULL);
memcpy(&t0, payload, sizeof t0);              /* on relit l'heure de départ */
double rtt_ms = (t1.tv_sec  - t0.tv_sec)  * 1000.0
              + (t1.tv_usec - t0.tv_usec) / 1000.0;  /* RTT en millisecondes */
```

Le `memcpy` n'est pas un détail de coquetterie : l'horodatage se trouve à un offset quelconque dans un tampon d'octets, possiblement mal aligné pour un accès direct à une `struct timeval` — on le recopie donc dans une variable correctement alignée avant de l'exploiter. Le RTT obtenu alimente quatre accumulateurs (minimum, maximum, somme, somme des carrés) qui serviront aux statistiques finales.

## La prise brute, et le privilège qu'elle exige

Pour tout cela, `ping` a besoin d'une **prise réseau** d'un genre inhabituel. Les programmes ordinaires ouvrent des sockets de haut niveau (`SOCK_STREAM` pour TCP, `SOCK_DGRAM` pour UDP) où le noyau bâtit lui-même les en-têtes. `ping`, qui doit composer son propre message ICMP, ouvre un **raw socket** :

```c
fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
```

Avec cette prise, `ping` n'écrit que la partie ICMP ; c'est encore le noyau qui, à l'émission, coiffe le paquet de son en-tête IP (il calcule l'adresse source, le checksum d'en-tête, etc.). Mais à la **réception**, surprise : le noyau livre le paquet *complet*, en-tête IP inclus. Il faut donc sauter cet en-tête pour atteindre l'ICMP — et sa longueur est variable (20 octets au minimum, davantage s'il porte des options). On la lit dans le champ *IHL* de l'en-tête IP, exprimé en mots de 32 bits, qu'on multiplie par 4 : `hlen = ip->ip_hl << 2`. C'est aussi dans cet en-tête IP reçu qu'on récupère le **TTL** affiché et l'adresse du répondeur.

Ce pouvoir a un prix : ouvrir un `SOCK_RAW` est **réservé**. Il faut être `root`, ou détenir la *capability* `CAP_NET_RAW` — un droit fin qui accorde précisément l'accès aux sockets bruts sans donner les pleins pouvoirs. Linux offre une issue de secours : si le `SOCK_RAW` est refusé (`EPERM`), on se rabat sur un `SOCK_DGRAM` ICMP, la « *ping socket* », ouverte aux utilisateurs d'une plage de groupes autorisés (`net.ipv4.ping_group_range`) — c'est ce qui permet souvent de *pinger* sans `sudo`. Inconvénient : sur cette prise, c'est le noyau qui fixe l'identifiant, et notre PID y perd son rôle. Enfin, le programme bien élevé **abandonne ses privilèges** dès la prise ouverte (`setuid(getuid())`) : il n'en a plus besoin, autant ne pas les conserver — une précaution élémentaire pour un binaire qui tourne souvent en `root`.

## Le TTL, ou comment un paquet finit par mourir

Le **TTL** (*time to live*) est un octet de l'en-tête IP qui borne la vie d'un paquet. Chaque routeur traversé le **décrémente d'une unité** avant de relayer ; tombé à zéro, le paquet est jeté, et le routeur renvoie à l'expéditeur un message ICMP *Time Exceeded* (type 11). C'est le garde-fou contre les boucles de routage : un paquet égaré s'éteint au lieu de tourner indéfiniment. `ping` ne fixe pas de TTL par défaut (il laisse le noyau choisir le sien), mais l'option `--ttl N` le pose sur le socket via `setsockopt(IPPROTO_IP, IP_TTL, …)`. Et ce même mécanisme, détourné, fait toute la magie de `traceroute` : en émettant des paquets aux TTL croissants (1, 2, 3…), on force chaque routeur du chemin à se dénoncer par un *Time Exceeded*, et l'on reconstitue la route saut par saut.

## Le verdict : les statistiques

À l'arrêt, `ping` tire le bilan. Il connaît le nombre de paquets émis et reçus ; le **taux de perte** est un calcul entier, volontairement tronqué : `(émis − reçus) × 100 / émis`. Puis, s'il a chronométré, il résume les temps de trajet par quatre nombres — minimum, moyenne, maximum, et **écart-type**. La moyenne et l'écart-type se déduisent des accumulateurs entretenus à chaque réception : avec `n` réponses, `somme` et `somme_des_carrés`,

```
moyenne   = somme / n
variance  = somme_des_carrés / n − moyenne²
écart-type = √variance
```

— la formule de la variance « moyenne des carrés moins carré de la moyenne », qui évite de garder tous les échantillons. La racine carrée elle-même est calculée à la main, par itérations de Newton, dans l'implémentation de référence. Tout cela s'imprime selon un format figé que `ft_ping` devra reproduire à l'identique, jusqu'à la ponctuation décimale — car `setlocale` peut, selon la langue du système, transformer le point en virgule.

## Au-delà de l'écho

L'écho n'est pas la seule question qu'ICMP sache poser, et la version GNU de `ping` sait en formuler deux autres. La requête d'**horodatage** (type 13, réponse type 14) demande à la cible l'heure qu'il est chez elle, mesurée en millisecondes depuis minuit UTC. La requête de **masque d'adresse** (type 17, réponse type 18, que l'option `--mask` désigne aussi) interroge une machine sur le masque de son sous-réseau — vestige d'une époque où un hôte pouvait l'ignorer au démarrage. Rares aujourd'hui, ces variantes existent néanmoins, et un clone fidèle se doit de les offrir.

Car c'est de fidélité qu'il s'agit. `ft_ping` ne réinvente pas ce protocole : il le rejoue. La boussole que je me suis fixée est l'implémentation de GNU **inetutils-2.0**, que je cherche à imiter jusque dans ses moindres détails — les mêmes options, les mêmes messages, les mêmes chiffres à l'écran. Comprendre ce que fait `ping` était la première marche ; la suivante fut de décider comment *nommer* et *ranger* tout cela dans le code — mais c'est une autre histoire, celle de l'en-tête où vit désormais le vocabulaire du programme.

## Sources

- [RFC 792 — Internet Control Message Protocol](https://www.rfc-editor.org/rfc/rfc792) (types ICMP, format Echo Request / Reply, Time Exceeded)
- [RFC 791 — Internet Protocol](https://www.rfc-editor.org/rfc/rfc791) (en-tête IP, champ IHL, TTL)
- [RFC 1071 — Computing the Internet Checksum](https://www.rfc-editor.org/rfc/rfc1071) (addition en complément à un sur 16 bits)
- `struct icmphdr`, `struct iphdr` — `<netinet/ip_icmp.h>`, `<netinet/ip.h>` (glibc)
- `man 7 raw` (raw sockets, réception de l'en-tête IP), `man 7 capabilities` (`CAP_NET_RAW`)
- GNU inetutils-2.0, sources `ping/` et `libicmp/` — l'implémentation de référence (checksum, horodatage embarqué, statistiques)
