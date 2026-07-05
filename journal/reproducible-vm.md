# Une machine qu'on peut jeter

Le programme que je développe doit tourner sur une Debian précise — pas celle de ma machine de tous les jours. La solution habituelle, c'est une machine virtuelle : un ordinateur simulé, avec son propre système, isolé du mien. Mais une VM installée à la main est un objet fragile : impossible de reproduire à l'identique, et perdue le jour où on la supprime. J'ai donc visé autre chose — une seule commande qui *fabrique* la VM de zéro, l'installe et l'équipe, pour pouvoir la jeter et la recréer sans y penser.

Le trajet complet, de l'image officielle à la machine prête à l'emploi, tient en six étapes enchaînées :

```mermaid
flowchart LR
    I["ISO Debian<br/>officielle"] --> P["preseed<br/>(dans l'initrd)"]
    P --> R["ISO remasterisée<br/>(reste amorçable)"]
    R --> V["VM créée<br/>(VBoxManage, headless)"]
    V --> S["attente SSH<br/>(installe + redémarre)"]
    S --> A["Ansible<br/>(les outils)"]
```

## Maquiller l'image d'installation

Le point de départ est l'image d'installation officielle de Debian (un fichier `.iso`). Elle est conçue pour une installation interactive — exactement ce que je veux éviter. Je la *remasterise* donc : j'y glisse mes réponses préparées à l'avance (le *preseed*, sujet d'un autre article) et je modifie son menu de démarrage pour qu'il lance l'installation aussitôt, sans attendre qu'on appuie sur une touche.

Le détail qui m'a d'abord échappé, c'est *où* déposer ces réponses. Les poser à côté des fichiers de l'ISO ne suffit pas : l'installateur Debian, très tôt dans son démarrage, cherche un `/preseed.cfg` à la racine de son **initrd** — ce petit système de fichiers que le noyau charge en mémoire avant tout le reste. Il faut donc l'injecter *là*, à l'intérieur d'une archive compressée. Le script la décompresse (`gunzip`), la rend inscriptible — `xorriso` l'a extraite en lecture seule —, y **ajoute** le fichier avec `cpio` sans toucher au reste, puis la recompresse. Le preseed voyage ainsi dans les entrailles de l'image, à l'endroit exact où l'installateur ira le lire.

L'opération est délicate, car une image d'installation est aussi *amorçable* : le firmware de la machine sait démarrer dessus, et la moindre maladresse casse cette propriété. L'outil `xorriso` permet heureusement de ne remplacer que les quelques fichiers voulus tout en **rejouant** (`replay`) la configuration d'amorçage d'origine — l'image reste démarrable sans que j'aie à la reconstruire.

## Deux firmwares, deux amorces

Encore faut-il que ce démarrage automatique fonctionne sur les deux familles de firmware qu'on croise aujourd'hui : l'ancien **BIOS** et l'**UEFI** moderne. Elles ne lisent pas le même fichier de configuration — le BIOS passe par *isolinux*, l'UEFI par *GRUB* —, si bien que je modifie les deux, avec les mêmes réglages : démarrer directement l'installateur en mode texte, en lui passant `auto=true priority=critical` pour qu'il ne pose aucune question.

Ces réglages transportent une décision inattendue : l'installation se fait en **anglais**, pas en français. La raison est terre à terre. Pour suivre l'installation, je détourne la console de la VM vers un fichier texte (une « console série »), et ce canal ne sait afficher que de l'ASCII. Or l'étape de choix de langue de Debian *rejette* le code `fr` dès lors qu'elle tourne sur un tel canal. J'installe donc en `en_US` — un moindre mal —, quitte à générer la locale française plus tard, au moment d'équiper la machine.

## Piloter VirtualBox sans la souris

VirtualBox s'utilise d'ordinaire à la souris, mais il offre aussi une commande, `VBoxManage`, qui fait tout sans interface : créer la VM, lui attribuer mémoire, processeurs et disque, y brancher l'image, puis la démarrer *headless* — sans fenêtre, en tâche de fond. Détail qui compte pour les ordinateurs de l'école : tout cela fonctionne en simple utilisateur, sans droits d'administrateur.

Un réglage mérite qu'on s'y attarde, car il évite un piège classique — la boucle de réinstallation. J'ordonne à la VM de tenter de démarrer sur le **disque d'abord**, sur le DVD ensuite :

```bash
VBoxManage modifyvm "$VM_NAME" --boot1 disk --boot2 dvd
```

Au tout premier démarrage, le disque est vierge : la machine « tombe » donc sur le DVD et lance l'installation. Une fois Debian installé sur le disque, ce même ordre le fait démarrer *lui* — et non plus le DVD. Sans cette astuce, la VM réinstallerait le système à chaque redémarrage, indéfiniment. Le réseau, lui, est en mode *NAT*, avec une redirection d'un port de ma machine vers le port SSH de la VM.

## Savoir quand c'est prêt

Un script ne « voit » pas une installation se terminer. Alors comment savoir que la VM est prête ? J'ai retenu un signal indirect mais fiable : tenter régulièrement de s'y connecter par SSH, jusqu'à ce que ça réponde.

```bash
until ssh_ready; do
  sleep 15
done
```

où `ssh_ready` lance une commande triviale sur la VM et échoue tant qu'elle ne répond pas. La beauté de ce signal, c'est tout ce qu'il **prouve** d'un coup : si SSH répond, alors le système est installé, il a redémarré, son réseau fonctionne, et son serveur SSH accepte ma clé. Quatre certitudes pour un seul test. (C'est ici que la clé déposée à la fin de l'installation prend tout son sens.) En parallèle, le script affiche en direct la console série de la VM : l'installation défile sous les yeux, et une échéance de trente minutes évite d'attendre pour rien si quelque chose se bloque.

## Équiper la machine : Ansible

La VM joignable, il reste à installer les outils de développement. Plutôt qu'une suite de commandes `apt install`, j'emploie `Ansible` : on y *décrit* l'état souhaité — « ces paquets doivent être présents » — et l'outil s'arrange pour l'atteindre. Son atout est l'*idempotence* : rejouer la même description n'y change rien si tout est déjà en place. La preuve se lit dans le compte rendu — un second passage annonce « 0 modifié ». C'est aussi cette étape qui génère la locale française absente de l'installation.

## Pourquoi pas un outil clé en main ?

On pourrait me demander pourquoi ne pas avoir pris `Vagrant`, l'outil de référence pour fabriquer des VM. J'y ai pensé, puis renoncé : il n'existe pas d'image Debian trixie officielle pour VirtualBox, et Vagrant lui-même serait une dépendance de plus à installer sur les machines de l'école, où je ne suis pas administrateur. Mon assemblage `VBoxManage` + preseed ne réclame rien que l'école n'ait déjà, et se comporte à l'identique chez moi et là-bas. Moins magique, mais à moi de bout en bout.

## Sources

- [Oracle VM VirtualBox — VBoxManage](https://www.virtualbox.org/manual/ch08.html) — `createvm`/`modifyvm`, l'ordre d'amorçage, la redirection de port NAT, le mode *headless*
- [GNU xorriso — manuel](https://www.gnu.org/software/xorriso/man_1_xorriso.html) — l'extraction (`osirrox`) et le `replay` du secteur d'amorçage
- [Debian — méthode de preseed et injection dans l'initrd](https://www.debian.org/releases/stable/amd64/apbs02) — pourquoi `/preseed.cfg` se charge depuis l'initrd
- [Ansible — documentation](https://docs.ansible.com/) — la description d'état et l'idempotence
