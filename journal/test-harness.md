# Le banc d'essai avant la pièce

Le programme ne fait toujours rien. Et pourtant, c'est le moment que j'ai choisi pour installer tout ce qui servira à le tester. Avant d'usiner une pièce, un atelier prépare le banc d'essai : le support, les comparateurs, l'étalon contre lequel on mesurera. C'est exactement ce que j'ai fait ici — monter l'appareillage de mesure pendant que l'établi est encore vide, pour que chaque morceau de code à venir puisse être vérifié dès sa première ligne.

## Tester d'abord, coder ensuite

Il y a deux façons d'aborder les tests. La plus répandue : écrire le programme, puis, si le temps le permet, ajouter quelques vérifications. La plus exigeante : poser les vérifications d'abord, et écrire le code pour les satisfaire. Cette seconde manière a un nom — le développement piloté par les tests — et un mérite : elle force à décider ce qu'on attend *avant* de se perdre dans le comment. Pour pouvoir travailler ainsi quand viendra le vrai code, il fallait que la machinerie de test soit déjà là, prête à accueillir le premier test au moment où j'écrirai la première fonction.

## Deux regards sur un programme

On peut examiner un programme de deux distances différentes.

De près, on inspecte ses **rouages** : telle fonction reçoit telle entrée, doit-elle bien rendre tel résultat ? On isole un calcul — une somme de contrôle, une moyenne — et on le confronte à des valeurs connues d'avance. Ce sont les tests *unitaires* : précis, rapides, ils pointent exactement la pièce fautive.

De loin, on observe le **comportement d'ensemble** : lancé avec ces arguments, le programme affiche-t-il ce qu'il faut, à la virgule près ? Ici on ne regarde plus l'intérieur, mais le résultat visible, qu'on compare à une référence. Ce sont les tests de *conformité* — et puisque mon `ping` doit imiter fidèlement un modèle existant, ils seront décisifs.

Les deux se complètent : les premiers garantissent que chaque rouage est juste, les seconds que l'assemblage se comporte comme l'original.

## Un juge de paix

Pour comparer, encore faut-il un modèle de référence incontestable. Le mien, c'est le `ping` d'une suite logicielle précise, dans une version précise — celle que mon programme doit reproduire. Mais on ne se contente pas de l'attraper au hasard : la machine en possède déjà un, *différent*, et il ne faut surtout pas l'écraser.

J'ai donc écrit un script qui va chercher le code source officiel de cette référence, **vérifie sa signature cryptographique** (la preuve qu'il provient bien de son auteur et n'a pas été altéré en route), le compile, et installe le résultat **dans un coin à part**, sous un nom dédié — le `ping` du système reste intact à côté. Le script est *reconstructible* : on peut l'effacer et le relancer, il refait tout à l'identique. J'ai ainsi mon juge de paix sous la main, toujours disponible, sans jamais perturber le reste de la machine.

Détail révélateur, vérifié en chemin : ce `ping` de référence affiche ses temps avec une **virgule** décimale et non un point. Rien d'anormal — le programme épouse les conventions régionales de la machine, et la mienne est réglée en français, où la virgule est le séparateur décimal d'usage. Sur un poste configuré en anglais, le même programme écrirait un point. Mon programme devra se comporter pareil : se plier à ces réglages plutôt que de figer un choix en dur. Quant à mes tests, ils devront fixer eux-mêmes une convention connue, pour comparer sur une base identique d'une machine à l'autre.

## Allumer le banc à vide

Comment vérifier que tout ce bel appareillage fonctionne, alors qu'il n'y a encore rien à mesurer ? En allumant le banc à vide. J'ai écrit un test *trivial* — il affirme qu'un égale un, rien de plus. Sa valeur n'est pas dans ce qu'il vérifie, mais dans le fait qu'il **s'exécute** : sa réussite prouve que toute la chaîne est en place — le framework de test compile, s'assemble, se lance, rend son verdict, et s'intègre à la commande de vérification globale. Le jour où j'écrirai un vrai test, je saurai que seule la nouveauté peut échouer.

Cet assemblage cache un piège classique. Un programme a un unique point d'entrée — la fonction par laquelle l'exécution commence. Or le framework de test apporte *le sien*. Mettre les deux dans le même exécutable, c'est provoquer une collision : lequel commande ? La parade consiste à bâtir l'exécutable de test avec tous les rouages du programme **sauf** son point d'entrée, en laissant celui du framework prendre les commandes. Une frontière nette entre le programme et son banc d'essai.

## Ne pas bâtir ce dont on n'a pas besoin

La recherche préparatoire avait dégagé quantité de techniques de test séduisantes — éprouver le programme avec des entrées aléatoires pour le faire craquer, inspecter les paquets réseau octet par octet, mesurer la part de code réellement exercée. La tentation était d'échafauder tout cela d'emblée.

J'ai résisté, et c'est peut-être ma meilleure décision sur ce point. Chacune de ces techniques vise une partie du programme **qui n'existe pas encore**. Installer leur outillage maintenant, ce serait bâtir des échafaudages autour du vide — du poids mort à entretenir, qui rouille avant d'avoir servi. J'ai donc posé seulement ce qui est utilisable tout de suite, et **consigné le reste par écrit** : chaque technique différée est notée dans un registre, avec la condition précise qui la rendra pertinente — « à mettre en place quand telle pièce existera ». Rien n'est oublié, rien n'est construit en avance. Le « pas encore » assumé et tracé vaut mieux que le « au cas où » qui encombre.

## Un outil têtu

Une dernière surprise, instructive. Je m'étais imposé, plus tôt, des règles de rigueur très strictes : le compilateur refuse la moindre conversion douteuse, un gardien bloque tout commit mal formé. Ces règles, taillées pour *mon* code, se sont retournées contre le code venu du framework de test : ses tournures internes, parfaitement légitimes chez lui, déclenchaient mes alarmes. Il a fallu desserrer l'étau — mais *précisément* au bon endroit, pour le seul code de test, sans rien relâcher pour le programme lui-même.

Le gardien des commits, lui, s'est révélé plus sévère que le contrôle de référence qu'il était censé refléter : il inspectait des fichiers que le contrôle officiel, lui, épargnait. Deux exigences censées dire la même chose divergeaient en silence. La leçon, que je note pour la suite : un garde-fou local n'a de valeur que s'il reproduit *exactement* la règle qu'il prétend protéger — sinon il finit par interdire ce que la règle autorise.

## Sources

- [Criterion — a C unit testing framework](https://github.com/Snaipe/Criterion)
- [GNU inetutils](https://www.gnu.org/software/inetutils/) (la référence de conformité)
- [Verifying GnuPG signatures](https://www.gnupg.org/gph/en/manual/x135.html) (authenticité d'une archive signée)
