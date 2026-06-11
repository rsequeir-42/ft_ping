# Un gardien hors de ma portée

J'avais déjà des gardiens. Chaque fois que je m'apprête à envoyer mon travail, des vérifications se déclenchent d'elles-mêmes et refusent de laisser filer ce qui n'est pas en ordre. Mais ces gardiens-là ont un défaut : c'est moi qui les tiens. Un mot suffit à les faire taire. Commode le jour où l'on est pressé — troublant si l'on y songe, car une barrière qu'on peut lever soi-même ne protège que tant qu'on le veut bien. Il me manquait un dernier gardien : un que je ne puisse pas amadouer, posté hors de ma portée.

## Un contrôle qu'on ne fait pas taire

Les garde-fous installés sur ma propre machine se déclenchent au bon moment, mais ils obéissent à celui qui les a posés. D'un mot, je les contourne — parfois à raison, souvent par facilité. Le jour où la facilité l'emporte, la vérification saute, et personne ne le voit.

La parade n'est pas d'ajouter un garde-fou de plus au même endroit : ce serait un gardien de plus que je tiens en laisse. C'est de déplacer le contrôle **ailleurs**, sur une machine que je ne commande pas, et de l'adosser à une règle que je ne peux pas enfreindre sans la défaire au grand jour. Un gardien n'a de valeur que s'il peut me dire non.

## Une machine neuve, à chaque fois

Le procédé porte un nom — l'intégration continue — et repose sur une idée sobre : à chaque changement que je propose, un service tiers loue une machine **vierge**, y installe tout l'outillage en partant de rien, et rejoue mon rituel de vérification depuis le début.

La nuance est essentielle. « Ça marche chez moi » ne vaut plus rien : la machine ne me croit pas sur parole, elle refait tout elle-même. Si la vérification passe sur un poste nu qu'elle a monté de ses mains, alors elle passe *vraiment* — pas par chance, pas grâce à un réglage oublié quelque part sur mon établi. Et ce qu'elle rejoue, c'est exactement la **porte unique** que je franchis chez moi : la même commande, les mêmes contrôles, le même verdict. Avoir tout regroupé derrière une seule porte, plus tôt, prend ici tout son sens — il n'y a qu'une chose à rejouer, et aucune place pour que le contrôle d'ici diffère de celui de là-bas.

## Sur le terrain de l'examen

Un détail pèse lourd : la machine sur laquelle je travaille n'est pas celle sur laquelle ce programme sera jugé. Je développe sur un système ; l'évaluation se fera sur un autre, voisin mais distinct. Ne vérifier que chez moi reviendrait à répondre à côté de la question.

J'ai donc demandé au gardien de refaire son contrôle **deux fois** : une dans un environnement proche du mien, une dans une reproduction fidèle du système d'examen — un *conteneur*, c'est-à-dire un petit monde isolé qui imite un autre système sur la même machine. Le même contrôle, sur les deux terrains. Si l'un des deux bronche — une mise en forme du code jugée différemment d'une version d'outil à l'autre, par exemple — je veux l'apprendre maintenant, pas le jour de l'examen.

## Se méfier des outils qu'on emprunte

Le gardien ne part pas de zéro : il s'appuie sur des briques toutes faites, des morceaux de tâche que d'autres ont écrits et partagés. C'est commode, mais c'est emprunter du code à des inconnus, et l'exécuter chez soi.

Or désigner une de ces briques par son nom est trompeur. Un nom peut être déplacé sans bruit vers une version modifiée — ce n'est pas une crainte théorique : une brique très répandue a déjà été détournée de la sorte pour tenter de dérober les secrets de quiconque l'utilisait. La parade consiste à n'emprunter chaque brique que par son **empreinte** : une longue signature qui désigne une version précise, figée, impossible à substituer en douce. Un nom est une promesse ; une empreinte est un fait.

Anecdote vérifiée en chemin : entre le moment où j'avais relevé l'empreinte d'une brique et celui où je l'ai effectivement posée, cette empreinte **avait changé**. La version derrière le nom n'était déjà plus la même. Rien de malveillant cette fois — mais la démonstration concrète qu'un nom ne fige rien, et qu'il fallait bien aller relever l'empreinte au dernier moment.

## Quand ma rigueur se retourne, encore

Une friction m'attendait, jumelle d'une que j'avais déjà rencontrée. Pour que le gardien rende un compte rendu lisible des tests, j'ai demandé à mon cadre d'essai d'écrire son rapport dans un fichier. Aussitôt, mon propre détecteur de fuites de mémoire — réglé au plus strict — a protesté : une poignée d'octets s'échappait.

Sauf que la fuite n'était pas dans *mon* code. Elle était dans le cadre d'essai lui-même, qui réserve un peu de mémoire pour lire ma demande et omet de la rendre avant de s'arrêter. Ma rigueur, taillée pour mon programme, se retournait contre un outil que j'emprunte — exactement comme la fois où mes règles de compilation les plus sévères avaient rejeté, non pas mon code, mais les tournures internes de ce même cadre. La leçon est la même, et je commence à la connaître : on ne baisse pas la garde partout pour faire taire une alarme. On l'abaisse **précisément** là où il faut — ici, ignorer les fuites qui proviennent de cet outil, et d'elles seules.

## Franchir ma propre barrière

Restait à donner au gardien son autorité. Tant qu'une règle ne l'y oblige pas, rien ne force à l'écouter ; je voulais l'inverse — que la ligne principale du projet n'accepte plus aucun changement sans son feu vert. Y compris les miens.

Désormais, même moi, je dois présenter mon travail comme une proposition, patienter le temps que le gardien ait tout recontrôlé sur ses machines neuves, et n'intégrer qu'au vert. La toute première proposition que je lui ai soumise fut, comme il se doit, celle qui l'installait lui-même : il s'est en quelque sorte contrôlé à sa naissance. Tout est passé du premier coup.

Ce « premier coup » n'est pas de la chance, et c'est peut-être la vraie morale. Chaque pièce confiée au gardien, je l'avais d'abord éprouvée à froid sur mon établi — les fichiers relus caractère par caractère, les règles essayées sur des exemples, le tout passé au crible avant le moindre envoi. La barrière n'a jamais eu à me dire non, parce que je ne lui ai tendu que ce dont j'avais déjà la preuve. Un gardien incorruptible ne sert pas à m'attraper en faute : il sert à garantir aux autres que je ne me suis pas attrapé moi-même trop tard.

## Sources

- [GitHub Actions](https://docs.github.com/en/actions) (le service d'intégration continue utilisé)
- [Security hardening for GitHub Actions](https://docs.github.com/en/actions/security-for-github-actions/security-guides/security-hardening-for-github-actions) (pourquoi épingler une brique par son empreinte)
- [Criterion](https://github.com/Snaipe/Criterion) (le cadre d'essai dont le rapport a réveillé la fuite)
