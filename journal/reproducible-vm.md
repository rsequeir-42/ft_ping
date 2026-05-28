# Une machine qu'on peut jeter

Le programme que je développe doit tourner sur une Debian précise - pas celle de ma machine de tous les jours. La solution habituelle, c'est une machine virtuelle : un ordinateur simulé, avec son propre système, isolé du mien. Mais une VM installée à la main est un objet fragile : impossible de reproduire à l'identique, et perdue le jour où on la supprime. J'ai donc visé autre chose - une seule commande qui *fabrique* la VM de zéro, l'installe et l'équipe, pour pouvoir la jeter et la recréer sans y penser.

## Maquiller l'image d'installation

Le point de départ est l'image d'installation officielle de Debian (un fichier `.iso`). Elle est conçue pour une installation interactive - exactement ce que je veux éviter. Je la *remasterise* donc : j'y glisse mon fichier de preseed, les réponses préparées à l'avance (j'en parle dans un autre article), et je modifie son menu de démarrage pour qu'il lance l'installation aussitôt, sans attendre qu'on appuie sur une touche.

L'opération est délicat, car une image d'installation est aussi *amorçable* : le firmware de la machine sait démarrer dessus, et la moindre maladresse casse cette propriété. L'outil `xorisso` permet heureusement de ne remplacer que les quelques fichiers voulus tout en **rejouant** la configuration d'amorçage d'origine - l'image reste démarrable, sur les deux firmwares courants (l'ancien BIOS et l'UEFI moderne), sans que j'aie à la reconstruire entièrement.

## Piloter VirtualBox sans la souris

VirtualBox s'utilise d'ordinaire à la souris, mais il offre aussi une commande, `VBoxManage`, qui fait tout sans interface : créer la VM, lui attribuer mémoire, processeurs et disque, y brancher l'image d'installation, puis la démarrer *headless* - sans fenêtre, en tâche de fond. Détail qui compte pour les ordinateurs de l'école : tout cela fonctionne en simple utilisateur, sans droits d'administrateur.

## Savoir quand c'est prêt

Un script ne « voit » pas une installation se terminer. Alors comment savoir que la VM est prête ? J'ai retenu un signal indirect mais fiable : le script tente régulièrement de s'y connecter par SSH. Tant que ça échoue, l'installation est en cours ; à la première connexion réussie, c'est que le système est installé, qu'il a redémarré, et que son serveur SSH répond - donc que tout est en place. (C'est ici que la clé déposée à la fin de l'installation prend tout son sens). Le réseau de la VM est en mode *NAT*, avec une redirection d'un port de ma machine vers le port SSH de la VM.

## Equiper la machine : Ansible

La VM joignable, il reste à installer les outils de développement. Plutôt qu'une suite de commandes `apt install`, j'emploie `Ansible` : on y *décrit* l'état souhaité - « ces paquets doivent être présents » - et l'outil s'arrange pour l'atteindre. Son atout est l'*idempotence* : rejouer la même description en change rien si tout est déjà en place. La preuve se lit dans le compte rendu - un second passage annonce « 0 modifié ».

## Pourquoi pas un outil clé en main ?

On pourrait me demander pourquoi ne pas avoir pris `Vagrant`, l'outil de référence pour fabriquer des VM. J'y ai pensé, puis renoncé : il n'existe pas d'image Debian trixie officielle pour VirtualBox, et Vagrant lui-même serait une dépendance de plus à installer sur les machines de l'école, où je ne suis pas administrateur. Mon assemblage `VBoxManage` + preseed ne réclame rien que l'école n'ait déjà, et se comporte à l'identique chez moi et là-bas. Moins magique, mais à moi de bout en bout.

## Sources

- [Oracle VM VirtualBox — VBoxManage](https://www.virtualbox.org/manual/ch08.html)
- [GNU xorriso — manuel](https://www.gnu.org/software/xorriso/man_1_xorriso.html)
- [Ansible — documentation](https://docs.ansible.com/)