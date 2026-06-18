# Des interrupteurs et une promesse

Si `ft_ping.h` est la carte du programme, `options.h` en est le mode d'emploi : il déclare comment une ligne de commande devient des décisions, et le vocabulaire — des bits — dans lequel ces décisions s'expriment. Le fichier tient en deux moitiés : une table de masques, puis trois fonctions. Prenons-les l'une après l'autre.

> Cet article suit `include/options.h` au plus près : à chaque évolution du fichier, il est mis à jour dans le même mouvement.

## Un interrupteur par option

Le programme reconnaît une dizaine d'options qui sont de simples bascules — présentes ou absentes, sans valeur. La façon naïve de les retenir serait de donner à chacune sa propre variable booléenne : `int verbose; int quiet; int numeric;` … Dix options, dix variables à déclarer, à passer, à remettre à zéro. `ft_ping` choisit une voie plus compacte : **un seul entier**, le champ `flags` de `t_options`, où chaque option occupe **un bit**. C'est l'objet de la première moitié d'`options.h` :

```c
#define OPT_FLOOD          0x001
#define OPT_INTERVAL       0x002
#define OPT_NUMERIC        0x004
#define OPT_QUIET          0x008
#define OPT_RROUTE         0x010
#define OPT_VERBOSE        0x020
#define OPT_IPTIMESTAMP    0x040
#define OPT_DEBUG          0x080
#define OPT_IGNORE_ROUTING 0x100
```

Pour saisir ces valeurs, il faut se représenter `flags` non comme un *nombre*, mais comme une **rangée d'interrupteurs** — les 32 bits d'un `unsigned int`, dont on n'utilise que les neuf premiers :

```
   flags  (unsigned int) — on ne montre que les 9 bits de poids faible

     bit:   8     7     6     5     4     3     2     1     0
          ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
          │ IGN │ DBG │ IPT │ VRB │ RRT │ QUI │ NUM │ INT │ FLD │
          └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
            0x100 0x080 0x040 0x020 0x010 0x008 0x004 0x002 0x001
```

Chaque masque `OPT_*` est une **puissance de deux** : un nombre dont l'écriture binaire ne comporte qu'un *seul* bit à 1. `0x001` désigne le bit 0, `0x002` le bit 1, `0x004` le bit 2… jusqu'à `0x100`, le bit 8. Neuf masques, neuf bits qui ne se chevauchent jamais — chacun son interrupteur. L'hexadécimal n'est pas un caprice : un chiffre hexadécimal vaut exactement quatre bits, si bien que la suite `1, 2, 4, 8, 10, 20, 40, 80, 100` se lit, pour qui a l'habitude, comme « un bit qui glisse d'un cran vers la gauche » — bien plus parlant que son équivalent décimal `1, 2, 4, …, 256`.

Trois opérations binaires suffisent alors à tout faire. **Allumer** un interrupteur, c'est un *ou* binaire (`|`), qui pose le bit voulu sans toucher aux autres :

```c
out->flags |= OPT_VERBOSE;   /* le bit 5 passe à 1 ; les huit autres inchangés */
```

**Vérifier** un interrupteur, c'est un *et* binaire (`&`), qui isole le bit :

```c
if (out->flags & OPT_QUIET) { /* -q a été demandé */ }
```

Ici une confusion guette, et il vaut mieux la dissiper : `out->flags & OPT_QUIET` ne vaut **pas** `1` quand l'option est présente — il vaut `OPT_QUIET`, c'est-à-dire `0x008`. Tout ce qui compte est que le résultat soit *non nul* (vrai) si le bit est posé, *nul* (faux) sinon ; c'est pourquoi on l'emploie tel quel dans un `if`, sans comparer à quoi que ce soit. **Éteindre** un interrupteur, enfin, c'est un *et* avec le complément du masque (`flags &= ~OPT_X` : `~OPT_X`, ce sont tous les bits à 1 sauf celui-là).

Un exemple chiffré vaut mieux qu'un long discours. La commande `ft_ping -dnv host` allume trois interrupteurs — `-d`, `-n`, `-v` :

```
   OPT_DEBUG   0x080   (bit 7)
   OPT_VERBOSE 0x020   (bit 5)
   OPT_NUMERIC 0x004   (bit 2)
   ──────────────────  l'union (|) de leurs bits :
   flags =     0x0A4   →  1010 0100  en binaire (bits 7, 5 et 2 allumés)
```

`flags` vaut alors `OPT_DEBUG | OPT_NUMERIC | OPT_VERBOSE`, et aucun des trois ne recouvre un autre — c'est toute la vertu des puissances de deux. Comme `flags` est un `unsigned int` (garanti d'au moins 16 bits par la norme, 32 en pratique), il reste de la place pour bien plus d'options que `ping` n'en compte ; et l'arithmétique **non signée** évite les pièges du bit de signe lors des décalages.

## Le rôle de chaque bascule

Reste à dire ce que chaque interrupteur *commande*. On peut les ranger en trois familles.

**Ce qui change l'affichage.**

- `-v` / `--verbose` (`OPT_VERBOSE`) — une sortie plus bavarde : `ping` y signale notamment les paquets ICMP qu'il reçoit *sans* qu'ils répondent à ses échos (messages d'erreur, paquets d'un autre type), qu'il tairait par défaut. Précieux pour comprendre pourquoi une cible reste muette.
- `-q` / `--quiet` (`OPT_QUIET`) — l'inverse : on supprime la ligne affichée à chaque réponse, pour ne garder que la ligne d'en-tête initiale et le résumé statistique final. Le mode d'un script qui ne veut que le bilan.
- `-n` / `--numeric` (`OPT_NUMERIC`) — désactive la résolution inverse. Par défaut, `ping` interroge le DNS pour afficher le *nom* d'hôte derrière chaque adresse ; avec `-n`, il s'en tient aux adresses numériques — plus rapide, et sans dépendre d'un serveur de noms.

**Ce qui agit sur le socket ou sur les options IP du paquet.**

- `-d` / `--debug` (`OPT_DEBUG`) — pose l'option socket `SO_DEBUG`, qui demande au noyau d'activer une trace de bas niveau sur la connexion. L'effet est surtout interne, rarement visible à l'écran.
- `-r` / `--ignore-routing` (`OPT_IGNORE_ROUTING`) — pose l'option socket `SO_DONTROUTE` : le paquet est émis directement sur l'interface du réseau local, sans consulter les tables de routage. Une façon de vérifier qu'une machine est bien sur le lien physique, en court-circuitant les routeurs.
- `-R` / `--route` (`OPT_RROUTE`) — active l'option IP *Record Route* (RFC 791) : chaque routeur traversé inscrit son adresse dans un espace réservé de l'en-tête IP, que `ping` affiche au retour. Cet espace est minuscule — neuf adresses tout au plus — ce qui réserve l'option aux chemins courts.
- `--ip-timestamp` (`OPT_IPTIMESTAMP`) — active l'option IP *Timestamp* (RFC 791), dans un mode passé en argument : `tsonly` (les horodatages seuls) ou `tsaddr` (horodatages *et* adresses). Chaque routeur y inscrit son heure de passage.

**Le régime intensif.**

- `-f` / `--flood` (`OPT_FLOOD`) — `ping` cesse d'attendre l'intervalle : il tire un paquet dès qu'une réponse arrive — ou toutes les dix millisecondes si elles se font attendre —, affichant un point à chaque départ qu'il efface à chaque retour. Le tapis de points se vide quand le réseau suit, s'épaissit quand il sature. Parce qu'il peut justement saturer un lien, le flood est **réservé au superutilisateur**.

**Deux faux interrupteurs.** Reste à lever une bizarrerie de la table : `OPT_INTERVAL` (`-i`) et `OPT_IPTIMESTAMP` (`--ip-timestamp`) accompagnent des options qui prennent un *argument* (une durée, un mode). Leur bit ne dit donc pas « activé » — une durée n'est pas un interrupteur — mais « **fourni** ». Il sert à trancher une ambiguïté : l'utilisateur a-t-il vraiment écrit `-i 0.5`, ou bien la valeur courante est-elle simplement le défaut ? Le bit répond ; la *valeur*, elle, est rangée à part, dans le champ `interval` de `t_options`. Bit d'un côté (présence), valeur de l'autre (contenu) : ne pas confondre les deux.

## Une fonction qui ne sort jamais

La seconde moitié du fichier déclare trois fonctions. La principale est `options_parse` :

```c
int options_parse(int argc, char **argv, t_options *out);
```

Sa signature *est* son contrat. Elle reçoit la ligne de commande brute (`argc`, `argv`) et un `t_options` à remplir (`out`), et ne renvoie pas un résultat mais un **code de statut** — un `int`. Trois propriétés, promises dans l'en-tête, en font une fonction docile.

Elle est **réentrante** : avant toute chose, elle remet `*out` à ses valeurs par défaut. On peut donc la rappeler autant de fois qu'on veut sur le même objet, sans que l'analyse précédente ne déteigne sur la suivante.

Elle **ne garde aucun état global** : tout transite par ses paramètres, si bien que deux analyses ne pourraient pas se gêner.

Et surtout, elle **n'appelle jamais `exit()`**. C'est le point cardinal. Une erreur de ligne de commande ne tue pas le programme depuis les entrailles du parseur : elle **remonte** sous la forme d'un code de retour — `0` si tout va bien, `EX_USAGE` (la valeur `64`, empruntée à `<sysexits.h>`, le standard des codes de sortie) en cas d'usage incorrect. C'est `main` qui tranchera. Cette retenue a deux vertus. Elle rend le décodage **testable** : un test peut lui soumettre des centaines de lignes de commande et inspecter chaque `t_options` produit, sans qu'un `exit` intempestif ne fasse tomber toute la batterie. Et elle **sépare** nettement la mécanique du décodage de la politique de sortie — l'une décide *ce qui a été demandé*, l'autre *ce qu'on en fait*.

## Afficher sans décider

Restent deux fonctions :

```c
void options_help(const char *prog);
void options_usage(const char *prog);
```

Elles écrivent, sur la sortie standard, la liste d'aide complète et le court message d'usage. Pourquoi les déclarer ici, à part ? Parce que `options_parse`, lorsqu'il rencontre `--help`, `--usage` ou `--version`, ne **fait** rien de plus que noter l'intention dans le champ `action` du record (les valeurs `ACT_HELP`, `ACT_USAGE`… de la carte des types). C'est ensuite `main` qui, au vu de cette action, appelle la bonne fonction d'affichage puis sort proprement. Le décodage *enregistre*, l'appelant *agit* — exactement comme pour les erreurs. Le paramètre `prog` sert à forcer le nom court du programme (`ft_ping`) dans le texte affiché, pour que la sortie reste identique quel que soit le chemin par lequel le binaire a été lancé.

## Une interface, pas une mécanique

Tout ce qu'`options.h` décrit, c'est un **contrat** : voici le vocabulaire — les bits —, voici les fonctions et ce qu'elles garantissent. *Comment* ce contrat est honoré — le recours à GNU `argp`, la table d'options qui mime inetutils, la fonction de rappel qui remplit le record — appartient au fichier d'implémentation, `options.c`, et a sa propre page. Cette frontière entre le *quoi* (l'en-tête, que tout le programme peut lire) et le *comment* (le source, compilé une seule fois) est précisément la raison d'être d'un fichier d'en-tête : on s'engage sur une interface stable, on garde la liberté de changer la mécanique derrière.

## Sources

- `<sysexits.h>` — les codes de sortie conventionnels (`EX_USAGE` vaut 64)
- `man 8 ping` (inetutils) — le rôle de chaque option en clair
- [RFC 791 — Internet Protocol](https://www.rfc-editor.org/rfc/rfc791) (options IP *Record Route* et *Timestamp*)
- `man 7 socket` — les options `SO_DEBUG` et `SO_DONTROUTE`
- `man 3 argp` — la base sur laquelle `options_parse` est bâtie (détaillée dans la page consacrée à `options.c`)
