# Se faire relire par des machines

Le programme ne fait toujours rien. Mon fichier principal se contente de se terminer proprement, et c'est très bien ainsi : avant d'écrire la première ligne sérieuse de `ping`, j'ai voulu installer ce qui me relira ensuite. Non pas des relecteurs humains, mais des outils - des programmes dont le seul métier est de traquer dans mon code ce que l'oeil laisse passer : une accolade mal placée, un bug latent, une erreur de mémoire invisible. L'idée est simple : déléguer la vigilance à des machines patientes, pour garder mon attention là où elle compte.

## Poser les garde-fous d'abord

On installe souvent ces vérifications tard, quand le code est déjà touffu et que les remontées se comptent par centaines - autant dire qu'on les ignore. En les posant alors que le projet est encore vide, chaque règle s'applique dès la première ligne, et le code naît conforme au lieu d'être rattrapé de force. C'est un peu austère, mais c'est le même pari que pour le reste : ce qui est vérifié tôt coûte moins cher que ce qui est découvert tard.

## Un style qui ne se discute plus

La première vérification est la plus modeste : la mise en forme. L'indentation, les espaces, la place des accolades. Rien de tout cela ne change ce que fait le programme, mais un style cohérent rend le code lisible, et surtout met fin aux débats stériles. J'utilise un outil, `clang-format`, qui reformate le code selon une configuration que j'écris une fois pour toutes ; chacun peut écrire comme il respire, l'outil remet tout d'aplomb.

Encore faut-il le régler à son goût. J'étais parti d'un style répandu comme base, en pensant n'avoir que des broutilles à ajuster. Mauvaise surprise : dès le premier essai, l'outil a *refusé* mon fichier - non parce qu'il était mal écrit, mais parce que ce style de base voulait **compacter** ma fonction de trois lignes en une seule, à l'exact opposé de ce que je voulais. Le formateur défaisait mon style. Le même passage m'a d'ailleurs révélé que mon fichier ne se terminait pas par un retour à la ligne, ce petit caractère invisible qu'exige la tradition Unix. Deux réglages plus tard, tout rentrait dans l'ordre. La leçon valait l'agacement : un outil de mise en forme a, lui aussi, des choix par défaut qu'il faut ausculter sur son propre code plutôt que supposer.

## Lire sans exécuter

Vient ensuite l'analyse *statique* : examiner le code sans le lancer, comme un correcteur lit une copie. C'est étonamment puissant - on peut repérer un pointeur potentiellement nul, une valeur de retour ignorée, une conversion hasardeuse, le tout sans jamais exécuter une instruction.

J'ai choisi de croiser deux outils plutôt qu'un, parce qu'ils ne regardent pas avec les mêmes yeux. Le premier, `clang-tidy`, s'appuie sur les entrailles d'un compilateur et connaît donc finement la grammaire du langage. Le second, `cppcheck`, est un moteur indépendant qui privilégie la prudence et explore des combinaisons que le premier ignore. Là où l'un se tait, l'autre parle parfois ; les utiliser ensemble, c'est superposer deux filets aux mailles décalées. J'ai ajouté à cela l'analyseur intégré au compilateur lui-même, qui simule mentalement les chemins que prendrait le programme pour débusquer des fautes comme libérer deux fois la même ressource - un atout précieux pour du code réseau, où l'on jongle avec des connexions de bas niveau qu'il ne faut ni oublier ni fermer en double.

## Surveiller pendant que ça tourne

L'analyse statique a une limite : elle raisonne sur le texte, pas sur la vie réelle du programme. Pour le reste, il faut le faire tourner sous surveillance. C'est le rôle de `valgrind`, qui exécute le programme dans une sorte de machine virtuelle épiant chaque accès à la mémoire. Il attrape les fuites - de la mémoire réservée puis jamais rendue - mais surtout une faute insidieuse : la lecture d'une mémoire qu'on n'a pas encore remplie. Ce piège est fréquent quand on assemble des paquets réseau octer par octet, et il a la mauvaise habitude de passer entre les gouttes des autres gardes-fous. Valgrind, lui, ne le rate pas. C'est lent, donc réservé aux contrôles ponctuels, mais c'est un regard que rien d'autre ne remplace.

## Une seule porte

Réunir tous ces outils ne sert à rien si je dois me souvenir de les lancer un par un. Je les ai donc rassemblés derrière une seule commande, qui les enchaîne et s'arrête au premier qui proteste. Une porte unique : tant qu'elle ne s'ouvre pas, rien ne passe. Son intérêt profond viendra plus tard, quand un serveur d'intégration rejouera exactement la même commande à chaque envoi de code. Pas de « ça marche chez moi » : la machine de l'auteur et celle qui valide font, mot pour mot, le même contrôle. La commande devient un contrat.

## Des sentinelles à l'entrée

Reste à ne pas oublier d'ouvrir la porte. Pour cela, je me suis appuyé sur une mécanique de l'outil de gestion de versions : les *hooks*, des petits scripts qu'il déclenche automatiquement à des moments clés. L'un s'exécute juste avant d'enregistrer une modification et vérifie la mise en forme des fichiers touchés ; l'autre, avant d'envoyer le code vers le dépôt distant, rejoue la porte complète. Des sentinelles postées aux entrées.

Une honnêteté s'impose toutefois : ces sentinelles se contournent d'une simple option, et chacun peut désactiver un hook sur sa propre machine. Ce sont des audes au quotidien, pas des remparts. le vrai rempart, infranchissable celui-là, viendra le jour où le contrôle s'exécutera sur un serveur que personne ne peut court-circuiter. Les hooks rendent la rigueur confortable ; ils ne la garantissent pas.

## Sources

- [ClangFormat — Clang documentation](https://clang.llvm.org/docs/ClangFormat.html)
- [Clang-Tidy — Extra Clang Tools](https://clang.llvm.org/extra/clang-tidy/)
- [Cppcheck — manual](https://cppcheck.sourceforge.io/manual.html)
- [Static Analyzer Options — GCC (`-fanalyzer`)](https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html)
- [Memcheck — Valgrind manual](https://valgrind.org/docs/manual/mc-manual.html)
- [Customizing Git — Git Hooks](https://git-scm.com/book/en/v2/Customizing-Git-Git-Hooks)
