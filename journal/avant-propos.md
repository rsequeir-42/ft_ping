# Avant-propos

Ce dépôt contient ma réécriture de `ping`, le petit utilitaire qui mesure le temps qu'un message met à atteindre une machine et à en revenir. L'exercice, mené dans le cadre du cursus de l'école 42, consiste à le reconstruire depuis zéro en C, jusqu'à en reproduire fidèlement le comportement — à la virgule près — d'après une implémentation de référence. Ce journal raconte ce chantier. Mais pas par où l'on s'y attendrait.

Car au fil de ces premières pages, le programme lui-même ne fait encore rien : pas une ligne de réseau, pas un seul paquet envoyé. J'ai d'abord passé du temps à autre chose — monter un atelier et poser des garde-fous. Bâtir de quoi compiler le code, dresser une machine d'essai qu'on peut jeter et refaire à l'identique, recruter des relecteurs automatiques, installer un banc de tests, poster un gardien qui veille au loin. Tout cela avant d'écrire la première instruction utile du programme.

C'est un pari : qu'un atelier soigneusement préparé fait gagner plus de temps qu'il n'en coûte, et qu'on avance plus sereinement quand des machines vérifient à votre place ce qu'on finit toujours par oublier. Chaque article revient sur une de ces fondations — pourquoi je l'ai posée, comment, et les pièges rencontrés en chemin. J'ai tâché de les rendre lisibles par une personne curieuse, pas seulement par un spécialiste du domaine.

Les textes se suivent dans l'ordre où ils ont été vécus, et se répondent souvent ; mais chacun tient debout tout seul. Ils sont rangés en deux temps : d'abord monter l'atelier, ensuite poser les garde-fous. Le programme, lui, viendra après — et le code vit [sur GitHub](https://github.com/rsequeir-42/ft_ping).
