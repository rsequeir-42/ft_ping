# Répondre avant qu'on ne demande

Installer un système d'exploitation, c'est d'ordinaire un dialogue : la machine pose des questions - quelle langue, quel clavier, comment découper le disque, quel mot de passe - et un humain répond. Pour une machine virtuelle que je compte recréer souvent, ce dialogue devient une corvée. Je voulais qu'une installation se déroule de bout en bout sans que personne ne touche le clavier. La réponse de Debian à ce besoin porte un nom : le *preseed*, littéralement « pré-semer » les réponses.

## debconf, la mémoire des réponses

Avant de comprendre le preseed, il faut connaître `debconf`. C'est le mécanisme, propre à Debian, qui centralise toute la configuration : chaque question qu'un paquet pourrait poser y est enregistrée, avec sa réponse. L'installateur ne « pose » pas vraiment les questions de lui-même - il les lit dans `debconf`, et n'interroge l'utilisateur que pour celles dont la réponse manque.

Le preseed exploite exactement cela : c'est un fichier qui **remplit `debconf` à l'avance**. Si la réponse est déjà là, la question ne s'affiche pas. On préremplit toutes les réponses, et l'installateur traverse ses écrans sans jamais s'arrêter.

## Une grammaire de quatre mots

Chaque ligne d'un preseed suit le même moule : `<propriétaire> <question> <type> <valeurs>`. Un exemple parlant :

```
d-i partman-auto/method string regular
```

`d-i` est le propriétaire (`debian-installer`, l'installateur lui-même) ; `partman-auto/method` est le nom de la question - ici, la méthode de partitionnement ; `string` est le type de la réponse ; `regular` la valeur. On croise d'autres types au fil du fichier ; `boolean` pour un oui/non, `select` pour un choix dans une liste, `password` pour un secret.

C'est verbeux, mais limpide : chaque ligne répond à exactement une question, et le fichier se lit comme la liste des décisions qu'on aurait prises à la main.

## Le vrai piège : le moment

Là où j'ai buté, ce n'est pas la grammaire, c'est la *chronologie*. Toutes les réponses ne sont pas consultées au même instant. Certaines questions - la langue, le clavier - sont posées tout au début, avant même que l'installateur ait fini de charger le fichier de preseed. Les préremplir dans le fichier ne suffit donc pas : à la seconde où la question tombe, l'installateur ne l'a pas encore lu.

Un exemple concret, vécu. La langue de l'installation se déclare dans le fichier ainsi :

```
d-i debian-installer/locale string en_US.UTF-8
```

Or cette ligne n'est *jamais* consultée à temps : l'écran de choix de langue apparaît avant que le fichier ne soit monté. La même réponse doit alors être donnée autrement - en *paramètre de démarrage* du noyau, sur la ligne même qui lance l'installateur :

```
locale=en_US.UTF-8 keymap=us
```

Ces paramètres-là, écrits directement dans la configuration d'amorçage de l'image, sont disponibles dès la première seconde, avant tout montage de fichier. La règle qui s'est dégagée est simple : les toutes premières questions passent par les paramètres de démarrage, le fichier de preseed couvre tout le reste. Comprendre cette frontière m'a coûté quelques essais - l'installation s'arrêtait obstinément sur l'écran de langue, alors que la réponse était « pourtant dans le fichier ».

## La touche finale : late_command

Reste un besoin que le preseed ordinaire ne couvre pas : exécuter mes propres commandes une fois le système installé. Dans mon cas, déposer une clé SSH et accorder les droits d'administration à l'utilisateur, pour que la machine soit immédiatement pilotable à distance. C'est le rôle de `late_command` : une suite de commandes lancées à la toute fin, *dans* le système fraîchement installé (grâce à `in-target`, qui les exécute à l'intérieur de la future machine plutôt que dans l'environnement temporaire d'installation).

C'est la dernière ligne du fichier, et la plus puissante : le point d'entrée qui transforme une installation générique en *ma* machine prête à l'emploi.

## Sources

- [Debian — Automating the installation using preseeding](https://www.debian.org/releases/trixie/amd64/apbs02.en.html) (paramètres de boot, contenu du fichier)
- [DebianInstaller/Preseed — Debian Wiki](https://wiki.debian.org/DebianInstaller/Preseed)
