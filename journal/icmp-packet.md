# Écrire un paquet à la main

Jusqu'ici, mon programme s'adressait soit à un humain (l'aide, les messages
d'erreur), soit au système (résoudre un nom, ouvrir un socket). Cette étape
fabrique le premier message destiné au **réseau lui-même** : un *Echo Request*
ICMP — la « question » qu'un ping pose à une machine distante. Je ne l'envoie pas
encore ; je le **construis**, octet par octet, exactement tel qu'il apparaîtra sur
le câble.

## Anatomie d'un écho

Un Echo Request tient en très peu de choses : **8 octets d'en-tête**, suivis de
données de longueur libre. L'en-tête porte cinq champs :

- un **type** (la valeur `8` signifie « ceci est un echo request ») ;
- un **code** (`0`) ;
- une **somme de contrôle**, qui garantit l'intégrité du message ;
- un **identifiant** et un **numéro de séquence**.

| Octets | Champ | Rôle |
|:---:|---|---|
| 0 | type | `8` — « ceci est un echo request » |
| 1 | code | `0` |
| 2–3 | somme de contrôle | intégrité du message |
| 4–5 | identifiant | reconnaître mes réponses |
| 6–7 | numéro de séquence | numéroter mes questions |
| 8… | données | renvoyées telles quelles |

Ces deux champs — identifiant et séquence — sont mon fil d'Ariane. Sur une même machine, plusieurs
ping peuvent tourner en même temps, et le réseau est bruyant. Chaque réponse
renvoie l'identifiant et la séquence de la question qui l'a provoquée : c'est
ainsi que je saurai, plus tard, qu'une réponse reçue est **la mienne**, et à
**quelle** de mes questions elle répond. Les données qui suivent l'en-tête sont un
contenu quelconque, que la machine distante renvoie tel quel — je m'en servirai un
jour pour mesurer le temps d'aller-retour.

## Le premier bloc vraiment pur

Les deux modules précédents *touchaient le monde* : l'un interrogeait le DNS,
l'autre ouvrait un socket. Construire un paquet, à l'inverse, ne touche **rien** :
c'est une transformation **pure** — `(identifiant, séquence, données)` donne *une
suite d'octets*, un point c'est tout. Une fonction au sens mathématique : mêmes
entrées, mêmes sorties, à chaque fois, sans dépendre de l'heure, du réseau ou de
quoi que ce soit d'extérieur.

C'est une propriété précieuse. Elle rend ce code **entièrement vérifiable en
mémoire**, sans le moindre socket ni privilège, et de façon parfaitement
reproductible. Là où le module réseau restait à moitié dans l'ombre (son succès
exige un privilège que l'intégration continue n'a pas toujours), celui-ci
s'éprouve en pleine lumière.

## L'ordre des octets, un piège invisible

Un nombre de deux octets — l'identifiant, par exemple — peut s'écrire de deux
façons en mémoire selon la machine : certains processeurs rangent l'octet de poids
fort en premier, d'autres en dernier. Le réseau, lui, impose **un seul** ordre.
L'identifiant et la séquence passent donc par une petite conversion (`htons`) qui
range leurs octets dans l'ordre du réseau : `0x1234` devient la paire d'octets
`12 34` sur le fil, jamais `34 12`. Oublier cette conversion, c'est expédier un
message que personne, à l'autre bout, ne saura relire correctement.

## Une structure, garantie par le compilateur

Restait à écrire ces huit octets proprement. J'ai décrit l'en-tête par une petite
**structure aux champs nommés**, de sorte que le code se lise comme le format
lui-même, chaque champ à sa place :

```c
typedef struct s_icmp_echo {
  uint8_t  type;
  uint8_t  code;
  uint16_t checksum;
  uint16_t id;
  uint16_t seq;
} t_icmp_echo;
_Static_assert(sizeof(t_icmp_echo) == 8, "l'en-tête doit faire 8 octets");
```

Le piège classique : rien ne garantit qu'une structure occupe *exactement* la
taille de ses champs. Pour respecter certains alignements, un compilateur peut
glisser des octets de **bourrage** invisibles au milieu — et le message émis
serait alors décalé, illisible. J'ajoute donc une **assertion vérifiée à la
compilation** : « cette structure fait exactement huit octets ». Si cette égalité
cessait un jour d'être vraie, le programme **refuserait de compiler**. Une simple
hypothèse devient ainsi une garantie que la machine contrôle pour moi.

Pour déposer ces huit octets dans le tampon d'envoi, j'emploie une copie mémoire
franche (`memcpy`), et jamais une « superposition » de pointeur — qui reviendrait
à lire la structure à une adresse potentiellement mal alignée. C'est un bug
silencieux sur mon ordinateur, mais un vrai plantage sur des architectures plus
strictes. J'ai même **durci le compilateur** pour qu'il refuse désormais ce genre
de raccourci partout dans le projet.

## Prouver, octet par octet

Parce que la construction est pure, je peux la prouver au sens fort : je fabrique
un paquet à partir de valeurs connues, puis je compare le résultat à la suite
d'octets attendue, **un à un**.

Les valeurs de test ne sont pas prises au hasard. Je choisis des octets tous
différents — `0x1234` pour l'identifiant, `0xDEADBEEF` pour les données —
précisément pour que la moindre inversion d'ordre **saute aux yeux** : `0x1234`
doit produire `12 34`, et si je lisais `34 12`, l'erreur serait flagrante. Une
valeur « plate » comme `0x0000` ne révélerait rien du tout. L'hexadécimal, ici,
n'est pas une coquetterie : un octet s'y écrit avec exactement deux chiffres, si
bien que le tableau attendu se lit comme le paquet réel, aligné case par case —
et se compare directement à une capture réseau.

Trois cas suffisent à couvrir toute la fonction : un paquet normal, un paquet sans
données, et un tampon trop petit — que la fonction refuse proprement plutôt que de
déborder. Le module est éprouvé à cent pour cent, sans qu'un seul octet ne parte
jamais sur le réseau. Ce sera l'affaire des étapes suivantes : lui donner sa somme
de contrôle, puis, enfin, l'envoyer.

## Sources

- [RFC 792 — Internet Control Message Protocol](https://www.rfc-editor.org/rfc/rfc792) — le format de l'en-tête Echo/Echo Reply
- [icmp(7) — Linux manual](https://man7.org/linux/man-pages/man7/icmp.7.html) — la `struct icmphdr` et les constantes `ICMP_*`
- inetutils-2.0, `libicmp/icmp_echo.c` — la construction de référence (type, code, `htons` sur id/seq)
