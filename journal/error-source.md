# Imprimer sans guetter de réponse

L'article sur `error.h` posait le *contrat* : deux fonctions, une voix unique pour les erreurs, un format figé à l'octet. `error.c` en est la **réalisation** — et elle tient en trois lignes de vrai code. Trois lignes seulement ; mais l'une d'elles cache un petit geste qui, pour qui le rencontre la première fois, intrigue : un `(void)` posé devant un appel. C'est lui qui justifie cette page.

> Cet article suit `src/error.c` au plus près : à chaque évolution du fichier, il est mis à jour dans le même mouvement.

## Le code, en entier

Le fichier inclut son propre en-tête (`"error.h"`, le contrat qu'il doit honorer) et `<stdio.h>` (pour `fprintf` et `stderr`), puis définit les deux fonctions :

```c
void error_try_hint(const char *prog) {
    (void)fprintf(stderr, "Try '%s --help' or '%s --usage' for more information.\n", prog, prog);
}

void error_report(const char *prog, const char *message) {
    (void)fprintf(stderr, "%s: %s\n", prog, message);
    error_try_hint(prog);
}
```

La logique est limpide. `error_report` écrit d'abord la ligne « *programme* : *message* », puis **délègue** la seconde ligne à `error_try_hint` : il *compose* les deux lignes plutôt que de tout réécrire, ce qui garantit que le « Try … » est rigoureusement le même, qu'il suive un message ou qu'on l'appelle seul. C'est la factorisation annoncée dans l'article frère, vue ici à l'œuvre.

## `fprintf`, en deux mots

`fprintf` est le cousin de `printf` : même travail — formater du texte —, mais vers un **flux qu'on choisit**, passé en premier argument. Ici, c'est `stderr`, le flux d'erreur (et non `stdout`) — pour les raisons développées dans l'article sur `error.h`.

Le deuxième argument, la *chaîne de format*, contient des marqueurs `%s` : autant de trous à remplir par des chaînes, prises dans les arguments suivants, dans l'ordre. Dans `error_report`, `"%s: %s\n"` a deux trous, comblés par `prog` et `message`. Un détail mérite l'œil dans `error_try_hint` : sa chaîne contient elle aussi **deux** `%s` (`'%s --help'` … `'%s --usage'`), mais ils désignent le *même* nom de programme — d'où l'appel `fprintf(stderr, …, prog, prog)`, où `prog` apparaît deux fois. Le nom figure deux fois dans la phrase affichée, donc deux fois dans les arguments fournis.

## Le `(void)` : imprimer sans écouter la réponse

Voici le geste qui intrigue. `fprintf`, comme la plupart des fonctions de la bibliothèque standard, **répond** : elle renvoie le nombre d'octets écrits, ou un nombre **négatif** si l'écriture a échoué (un disque plein, un tube refermé à l'autre bout…). Un code méticuleux est censé regarder cette réponse.

Or nous la jetons — explicitement, en préfixant l'appel de `(void)`. Pourquoi ce choix, et pourquoi le rendre visible ?

Le *pourquoi* d'abord. Que ferait-on, au juste, d'un échec d'écriture **ici** ? Le rôle de ces fonctions est précisément d'**annoncer une erreur**. Si écrire ce message échoue, signaler le problème… sur le flux qui vient justement de défaillir n'aurait aucun sens. Il n'y a rien d'utile à tenter ; on ignore donc le retour, en pleine connaissance de cause.

Le *pourquoi visible* ensuite. Notre politique d'analyse statique (réglée, on l'a vu ailleurs, pour refuser qu'on laisse filer en silence la valeur de retour d'une fonction) signalerait un appel à `fprintf` dont on néglige le résultat — au cas où ce serait un oubli. Le `(void)` est la réponse à cette vigilance : il **acte** que l'oubli est délibéré. C'est une signature, pas une décoration — « je sais que cette fonction me répond, et je choisis de ne pas l'écouter ». L'image est celle d'une lettre postée sans accusé de réception : on assume de ne pas savoir si elle est arrivée, parce qu'on ne ferait rien de plus si elle ne l'était pas.

## Les apostrophes, à nu

On les voit enfin en clair, dans le code : `'%s --help'`. Des apostrophes droites, gravées dans la chaîne de format. *Pourquoi* elles, et pas le quoting qu'une bibliothèque produirait, a été creusé dans l'article sur `error.h` (figer le rendu face à la locale et coller à l'étalon). Ce que `error.c` ajoute n'est qu'un constat : ce choix n'est pas une option qu'on règle quelque part, c'est du texte *littéral* dans le code. Ce que nous écrivons est exactement ce qui sort — ni plus, ni moins, ni traduit.

## Si peu de code, est-ce normal ?

On pourrait s'étonner qu'un « module » se réduise à trois lignes. C'est précisément l'intention : une frontière d'entrée/sortie doit être **mince et unique**. Tout le soin n'est pas dans la quantité de code, mais ailleurs — dans le format reproduit à l'octet, et dans la petite discipline du `(void)`. Le jour où le réseau apportera ses propres messages, ils passeront, eux aussi, par un point aussi sobre que celui-ci.

## Sources

- `man 3 fprintf` — `fprintf`, sa chaîne de format, et sa valeur de retour (octets écrits, ou négatif en cas d'erreur)
- CERT C, règle `ERR33-C` — « ne pas ignorer la valeur de retour d'une fonction » : la règle que le `(void)` satisfait explicitement
- L'article sur `error.h` (« Dire l'erreur d'une seule voix ») — le *pourquoi* de `stderr`, du module unique et du quoting figé
