# Dire l'erreur d'une seule voix

`error.h` tient en trois fonctions — pas une de plus — mais il tranche une question qui paraît anodine et ne l'est pas : *comment* `ft_ping` annonce qu'il a échoué. Un format figé, une distinction nette entre deux sortes de fautes, et un détail d'apostrophe qui décide de la conformité à l'octet. Petit fichier, donc, mais qui mérite qu'on déplie chacun de ses choix.

> Cet article suit `include/error.h` au plus près : à chaque évolution du fichier, il est mis à jour dans le même mouvement.

## Trois fonctions, deux sortes de fautes

L'en-tête déclare exactement ceci :

```c
void error_report(const char *prog, const char *message);
void error_value(const char *prog, const char *format, ...) __attribute__((format(printf, 2, 3)));
void error_try_hint(const char *prog);
```

Derrière ces trois déclarations, une idée : `ft_ping` n'échoue pas pour une seule raison, mais pour **deux** sortes de fautes, et il ne les annonce pas de la même manière.

- Une **erreur d'usage** — un hôte manquant, une option mal formée — passe par `error_report` : la ligne « *programme* : *message* », **suivie** du conseil « Try … » (va voir l'aide, la commande est mal écrite). Un opérande d'hôte manquant produit donc exactement ces deux lignes :

```
ft_ping: missing host operand
Try 'ft_ping --help' or 'ft_ping --usage' for more information.
```

- Une **valeur invalide** — `-s 99999`, `-c abc` — passe par `error_value` : la même ligne « *programme* : *message* », mais **sans** le « Try » (l'aide ne dit pas les bornes, le conseil serait creux). C'est aussi la seule qui prend un **format** à la `printf` (le `...` et l'attribut qui le suit), car ces messages ont des trous à combler — « option value too big: **65399** ». L'article « Lire un nombre, et s'en méfier » détaille cette seconde voix.

La troisième fonction, `error_try_hint`, n'imprime que la ligne « Try … ». La factoriser évite de la réécrire à chaque point d'échec — et donc de risquer une variante (un mot, une apostrophe) qui trahirait l'uniformité. `error_report` n'est d'ailleurs qu'`error_value` suivie d'`error_try_hint` : un format de base commun, et seul le conseil final sépare les deux voix.

## Sur stderr, et à un seul endroit

Deux partis pris se cachent dans ces deux fonctions.

D'abord, tout part sur **stderr**, le flux d'erreur, jamais sur `stdout`. La distinction est une convention Unix essentielle : `stdout` porte la *marchandise* — ce que le programme produit d'utile, ici la liste d'aide ou les statistiques — tandis que `stderr` porte les *remarques* — diagnostics, avertissements, erreurs. Les séparer, c'est permettre à un script de capturer le résultat d'un côté (`ping … > resultats.txt`) sans y mêler les messages d'erreur, qui continuent de s'afficher à l'écran. Mélanger les deux flux rendrait l'un et l'autre inexploitables.

Ensuite, ces impressions sont **regroupées dans un seul module**, et ce n'est pas un hasard : en concentrant les `fprintf` ici plutôt que de les éparpiller, on garantit un format unique, et on se donne un point unique à vérifier. C'est aussi ce qui rend l'erreur **testable** — un test peut rediriger `stderr`, déclencher une erreur et comparer la sortie, sans que le reste du programme s'en mêle.

Une précision sur le partage du travail avec `argp` : la bibliothèque imprime elle-même ses propres diagnostics (« unrecognized option … » pour une option inconnue). `error.c` ne double pas ce travail ; il couvre les erreurs que `ft_ping` détecte **de son côté** — au premier chef, l'hôte manquant. Les deux voix doivent se ressembler, d'où l'intérêt de calquer notre format sur celui d'`argp`.

## Une histoire d'apostrophes

Le détail qui justifie à lui seul d'écrire ce message nous-mêmes : les **apostrophes** autour de `'ft_ping --help'`. Ce sont des apostrophes droites ASCII (`'`, le code 39), inscrites *en dur* dans la chaîne de format.

Pourquoi ne pas laisser une bibliothèque produire ce conseil et choisir, elle, ses guillemets ? Parce que ce choix n'est pas stable. Déroulons-le :

1. Les bibliothèques GNU savent « citer » un mot avec des guillemets **dépendant de la locale** : en environnement C pur, des apostrophes droites ; mais dans une locale UTF-8, parfois des guillemets typographiques courbes (`'…'`). Le même code, deux rendus.
2. Et même à locale fixée, la version glibc d'`argp` rend son guillemet ouvrant par un **accent grave** (`` ` ``), là où l'étalon — la version *gnulib* qu'embarque inetutils — met une apostrophe droite.

En écrivant la ligne nous-mêmes, avec des apostrophes droites littérales, on **fige** le rendu : identique quelle que soit la langue du système, et identique à l'étalon. Pour un projet qui vise la conformité à l'octet, ce n'est pas un détail cosmétique, c'est la différence entre « conforme » et « presque ».

Reste la part qu'on ne maîtrise pas, et il faut être honnête à son sujet. Pour les erreurs qu'`argp` gère *seul* — une option inconnue —, c'est encore lui qui compose le « Try … », avec son accent grave glibc. Deux chemins, donc, deux guillemets :

| Qui détecte l'erreur ? | Qui imprime le « Try… » ? | Guillemet |
|---|---|---|
| `ft_ping` (hôte manquant) | `error_try_hint` (notre code) | `'` |
| `argp` (option inconnue) | `argp`, côté glibc | `` ` `` |

La première ligne, on la maîtrise et elle est conforme ; la seconde nous échappe, et diverge de l'étalon de quelques octets. Cette divergence est connue, acceptée, et consignée dans le registre des décisions différées. `error.c` règle ce qui est à notre portée ; il ne prétend pas réécrire ce qui appartient à la bibliothèque.

## Le nom, passé de main en main

Dernier choix, discret mais cohérent avec tout le reste du programme : le nom (`prog`) est un **paramètre**, pas une variable globale. C'est le nom court (`ft_ping`, tiré de `argv[0]`) que `main` calcule une fois et tend à qui en a besoin. Le passer de la sorte garde le module **sans état** — rien à initialiser, rien à partager — exactement comme `options_parse` reçoit son `t_options` plutôt que d'aller le chercher dans une globale. Et cela garantit que le nom affiché est le même partout, quel que soit le chemin par lequel le binaire a été invoqué (un lien symbolique, un appel par chemin absolu…). Une voix, un nom, un format : l'erreur parle toujours de la même manière.

## Sources

- La convention `stdout` / `stderr` (la marchandise contre les remarques) — `man 3 stdio`
- GNU inetutils-2.0, `ping/` — le format des messages d'erreur servant d'étalon
- `man 3 argp` — les diagnostics qu'`argp` émet de lui-même
- `DEFERRED.md` (ce dépôt) — la divergence « Try » accent grave / apostrophe, assumée
