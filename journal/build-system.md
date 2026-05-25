# Construire avant d'écrire

Avant de recoder `ping`, il faut pouvoir le compiler. Et avant même d'avoir écrit la moindre ligne du vrai programme, je me suis arrêté sur une pièce qu'on a tendance à bâcler : le `Makefile`, ce fichier qui explique à la machine comment fabriquer l'exécutable à partir du code source. J'aurais pu me contenter de trois lignes qui « font compiler ». J'ai préféré prendre le temps, parce qu'un build négligé se paie plus tard, en erreurs silencieuses.

## Make ne devine rien

Un `Makefile` n'est pas un script qu'on déroule de haut en bas. C'est une *description de dépendances* : on y déclare « tel fichier se fabrique à partir de tels autres », et l'outil `make` se débrouille pour ne refaire que le strict nécessaire. Sa logique tient en une phrase : il compare les **dates de modification**. Si le résultat est plus récent que tout ce dont il dépend, il ne touche à rien.

Compiler un programme en C, c'est deux étapes : d'abord transformer chaque fichier source (`.c`) en un fichier *objet* (`.o`) — du code machine pas encore assemblé ; puis *lier* (link) tous ces objets en un seul exécutable. `make` orchestre ces deux temps.

J'ai choisi de ranger les objets **hors des sources**, dans un dossier `obj/` à part, plutôt qu'à côté des `.c`. Ça garde le code propre, et le nettoyage devient trivial : un seul dossier à effacer. Un détail, mais l'accumulation de détails propres fait un projet où l'on retrouve ses petits.

## Le piège que `make` ne voit pas

Vient le point le plus sournois. `make` raisonne sur les dépendances qu'on lui **déclare** — or il ne lit pas le C. Si je me contente de lui dire « `main.o` dépend de `main.c` », il ignore que `main.c` inclut des fichiers d'en-tête (`.h`). Le jour où je modifie un en-tête partagé sans toucher au `.c`, `make` ne recompile rien : il assemble alors des morceaux incohérents, et le programme part en vrille à l'exécution sans qu'aucune erreur de compilation ne m'ait prévenu. Un bug invisible, le pire genre.

La parade consiste à déléguer le travail au compilateur lui-même : avec les options `-MMD -MP`, il écrit à côté de chaque objet la liste exacte des en-têtes dont il dépend, et `make` réinjecte cette liste. Modifier un en-tête recompile dès lors précisément ce qu'il faut, ni plus ni moins. C'est invisible à l'usage, et c'est exactement pour ça que ça compte.

J'en ai aussi profité pour garantir une exigence du sujet : ne **rien** recompiler quand rien n'a changé. La subtilité tient à une règle d'écriture — faire dépendre l'exécutable des objets, et non porter la commande de liaison sur une cible « fictive » qui se réexécuterait à chaque appel. Bien posé, un second `make` répond simplement « rien à faire ».

## Demander au compilateur d'être sévère

Le compilateur sait repérer quantité de constructions douteuses, mais il se tait par défaut. Je lui ai demandé d'être le plus bavard possible et, surtout, de **traiter chaque avertissement comme une erreur** : au moindre warning, la compilation s'arrête. C'est une discipline un peu rude — il faut écrire un code irréprochable dès la première ligne — mais elle déplace les bugs du moment de l'exécution (tard, coûteux) vers celui de la compilation (tôt, gratuit). Plusieurs de ces options vont compter pour du code réseau, où l'on manipule des octets, des tailles et des conversions à longueur de temps.

## Deux manières de compiler, et un silence inattendu

J'ai prévu deux profils. Un profil *release*, optimisé, pour l'usage normal. Et un profil *debug* truffé d'instruments d'analyse — les *sanitizers*, des sondes que le compilateur insère dans le code pour détecter, à l'exécution, les fautes de mémoire (accès hors limites, usage après libération…) et les comportements indéfinis. C'est lent, donc réservé au débogage, mais redoutablement efficace pour traquer ce que l'œil ne voit pas.

En me documentant sur ce mode debug, je suis tombé sur un piège que je n'aurais jamais deviné seul. `ping` a besoin d'un privilège particulier pour ouvrir sa connexion réseau de bas niveau. La manière moderne de l'accorder, une *capability* (un droit fin, sans donner les pleins pouvoirs), a un effet de bord déconcertant : dès qu'un programme porte une telle capability, le noyau verrouille par sécurité une partie de son environnement — et le sanitizer, qui a besoin de lire cet environnement pour se configurer, **se tait alors silencieusement**. On croit l'analyse active alors qu'elle ne fait plus rien. La leçon, notée pour le jour où j'en aurai besoin : pour le binaire instrumenté, on passe par `sudo`, pas par cette capability.

Le résultat tient en peu de chose : un fichier de quelques dizaines de lignes, un programme encore vide qui se contente de se terminer correctement, et la certitude que tout ce qui viendra se posera sur des fondations qui ne mentent pas.

## Sources

- [GNU Make — Manual](https://www.gnu.org/software/make/manual/make.html) (variables implicites, règles, dépendances)
- [microHOWTO — Automatically generate makefile dependencies](https://www.microhowto.info/howto/automatically_generate_makefile_dependencies.html) (`-MMD`/`-MP`)
- [AddressSanitizer — Clang documentation](https://clang.llvm.org/docs/AddressSanitizer.html)
- [google/sanitizers — issue #784](https://github.com/google/sanitizers/issues/784) (le silence des sanitizers face aux capabilities)
