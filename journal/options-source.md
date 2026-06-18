# Une table et un aiguilleur

L'article sur `options.h` décrivait un *contrat* : un vocabulaire de bits, trois fonctions, et la promesse que rien ne s'arrête brutalement. `options.c` est la **mécanique** qui honore ce contrat. C'est un fichier dense : il s'appuie de bout en bout sur **GNU argp**, une bibliothèque puissante mais exigeante, dont chaque réglage compte. Je vous propose de le lire dans l'ordre où il se présente — du haut vers le bas — en m'arrêtant sur chaque notion, chaque mot-clé, chaque valeur. Ce sera long ; c'est le prix de l'exactitude.

## argp, en deux mots

Analyser une ligne de commande, c'est transformer un tableau de chaînes (`argv`) en décisions. La libc offre pour cela `getopt`, rudimentaire, et **`argp`**, bâtie *au-dessus* de `getopt`, bien plus complète : elle gère les options courtes (`-v`) et longues (`--verbose`), génère elle-même les messages d'aide et d'usage, regroupe et trie les options, et impose une discipline de code.

Le marché qu'elle propose est le suivant : **on déclare** ce qu'on attend (une table d'options), **on fournit** une fonction de rappel (un *callback*) qu'elle appellera pour chaque élément reconnu, et **on lance** la machine avec `argp_parse`. C'est le choix de `ft_ping` parce que l'étalon, inetutils-2.0, l'emploie aussi : passer par la même bibliothèque, c'est hériter des mêmes formulations d'aide et d'erreur, presque à l'octet.

`options.c` est donc organisé en trois temps que cet article suit : **la table** (ce qu'on attend), **`parse_opt`** (l'aiguilleur, appelé pour chaque option), et **les fonctions publiques** qui lancent argp et impriment l'aide.

## Une clé pour chaque option

Tout commence par les **inclusions**. Outre notre propre `options.h`, le fichier tire `<argp.h>` (la bibliothèque), `<stdio.h>`, `<stdlib.h>`, `<string.h>` (pour `memset`), `<strings.h>` (pour `strcasecmp` — c'est bien `strings.h`, au pluriel, et non `string.h`), `<sysexits.h>` (pour le code `EX_USAGE`), et nos `error.h` / `ft_ping.h`.

Vient ensuite une énumération discrète mais essentielle :

```c
enum {
  ARG_ECHO = 256,
  ARG_ADDRESS,
  ARG_TIMESTAMP,
  ARG_ROUTERDISCOVERY,
  ARG_TTL,
  ARG_IPTIMESTAMP,
  ARG_USAGE
};
```

Pour comprendre ces nombres, il faut savoir comment argp identifie une option. Chaque option porte une **clé** (`key`), un entier, qui sert à deux choses : c'est l'identifiant transmis à notre callback *et*, sous une condition, le caractère de l'option courte. La règle exacte d'argp est : une clé est traitée comme option courte si elle est strictement positive, inférieure ou égale à 255, **et** imprimable. Ainsi la clé `'v'` (valeur ASCII 118) devient l'option `-v` ; la clé `'?'` devient `-?`.

Mais certaines options longues n'ont **pas** d'équivalent court — `--echo`, `--ttl`, `--ip-timestamp`. Il leur faut tout de même une clé unique pour que le callback les distingue. La convention, qui découle directement de la règle précédente, est de choisir une valeur **au-delà de 255** : argp ne pourra jamais la confondre avec un caractère imprimable, donc aucune lettre courte ne sera créée, et la clé reste un identifiant propre. D'où l'`enum` qui démarre à `256`. Les valeurs suivantes (`257`, `258`…) sont attribuées automatiquement par le compilateur, ce qui garantit leur unicité sans qu'on ait à les compter.

## La table : décrire, sans agir

Le cœur déclaratif du fichier est un tableau de `struct argp_option`. Chaque entrée a **six champs**, dans cet ordre : `name`, `key`, `arg`, `flags`, `doc`, `group`.

- `name` est le nom long (sans les `--`).
- `key` est la clé décrite plus haut.
- `arg`, s'il est non nul, est le **nom de l'argument** que l'option réclame (affiché dans l'aide, par exemple `NUMBER`) ; à `NULL`, l'option ne prend pas d'argument.
- `flags` porte des drapeaux `OPTION_*`.
- `doc` est la ligne d'aide.
- `group` sert au tri (j'y reviens).

Une option-drapeau comme `--verbose` s'écrit donc, sans argument :

```c
{"verbose", 'v', NULL, 0, "verbose output", GRP + 1},
```

et une option à valeur comme `--count` nomme son argument :

```c
{"count", 'c', "NUMBER", 0, "stop after sending NUMBER packets", GRP + 1},
```

**Les en-têtes de section.** Une entrée dont *à la fois* `name` et `key` valent zéro, mais dont `doc` est non nul, n'est pas une option : c'est un **titre de groupe**, affiché en retrait au-dessus des options qui suivent. La convention argp veut qu'on le termine par deux-points :

```c
{NULL, 0, NULL, 0, "Options controlling ICMP request types:", GRP},
```

**Les groupes.** Le dernier champ, `group`, gouverne l'ordre d'affichage. Argp trie les options **alphabétiquement à l'intérieur** d'un même groupe, puis présente les groupes dans un ordre particulier : d'abord les numéros positifs croissants (`0, 1, 2, …`), **ensuite** les négatifs (`…, -2, -1`). Une valeur `0` signifie « hériter du groupe précédent ». Et le groupe `-1` est, par tradition argp, celui des options « automatiques » comme l'aide — c'est pourquoi nos trois dernières entrées y sont placées :

```c
{"help", '?', NULL, 0, "give this help list", -1},
{"usage", ARG_USAGE, NULL, 0, "give a short usage message", -1},
{"version", 'V', NULL, 0, "print program version", -1},
```

Pour piloter cet ordre proprement, le fichier emploie une astuce de préprocesseur : un `#define GRP 0` ouvre une section, les options y sont rangées à `GRP + 1`, puis `#undef GRP` / `#define GRP 10` ouvre la suivante, et ainsi de suite par paliers de dix. On reproduit ainsi la disposition exacte de l'aide d'inetutils sans coder les numéros en dur partout.

**Une option cachée.** `--router` porte le drapeau `OPTION_HIDDEN` :

```c
{"router", ARG_ROUTERDISCOVERY, NULL, OPTION_HIDDEN, "send ICMP_ROUTERDISCOVERY packets (root only)", GRP + 1},
```

`OPTION_HIDDEN` (valeur `0x2`) signifie que l'option **fonctionne** — le callback la reçoit — mais n'apparaît dans **aucun** message d'aide. C'est le statut idéal d'une fonctionnalité déclarée mais pas encore prête.

**Le terminateur.** Comme argp ne reçoit pas la taille du tableau, il faut un marqueur de fin : une entrée dont les quatre champs `key`, `name`, `doc` et `group` sont nuls. D'où la dernière ligne, à ne pas confondre avec un en-tête de groupe (qui, lui, a un `doc`) :

```c
{NULL, 0, NULL, 0, NULL, 0}};
```

## Le décor : usage, documentation, adresse de bug

Trois éléments complètent la déclaration. D'abord deux chaînes :

```c
static const char args_doc[] = "HOST ...";
static const char doc[] = "Send ICMP ECHO_REQUEST packets to network hosts."
                          "\vOptions marked with (root only) are available "
                          "only to superuser.";
```

`args_doc` décrit les **arguments non-option** attendus — ici un ou plusieurs hôtes — et n'apparaît que dans la ligne `Usage:`. `doc` est le texte d'accompagnement, avec une subtilité : le caractère **`\v`** (tabulation verticale) le coupe en deux. Ce qui précède s'affiche **avant** la liste des options, ce qui suit s'affiche **après**. Une seule chaîne, deux emplacements.

Ensuite, une variable globale au nom imposé :

```c
const char *argp_program_bug_address = "<https://github.com/rsequeir-42/ft_ping/issues>";
```

`argp` reconnaît ce nom précis : si on définit `argp_program_bug_address`, son contenu est imprimé en bas de l'aide, dans une phrase du type « Report bugs to … ». C'est là que nous divergeons sciemment d'inetutils, qui pointe vers son propre traqueur : nous renvoyons vers le nôtre. (Il existe une variable sœur, `argp_program_version`, qui ferait apparaître un `--version` automatique ; nous ne la définissons **pas**, car nous gérons la version nous-mêmes.)

## decode_type : lire un nom de sonde

Avant le callback, une petite fonction traduit un nom de type de requête en valeur de l'énumération `t_ping_type` :

```c
static t_ping_type decode_type(const char *name) {
  if (strcasecmp(name, "echo") == 0) {
    return PING_ECHO;
  }
  if (strcasecmp(name, "timestamp") == 0) {
    return PING_TIMESTAMP;
  }
  ...
  return PING_ECHO;
}
```

`strcasecmp` compare deux chaînes **sans tenir compte de la casse** : `echo`, `Echo` et `ECHO` sont équivalents — c'est le comportement d'inetutils. C'est cette fonction qui vit dans `<strings.h>`, d'où l'inclusion séparée signalée plus haut.

## parse_opt : l'aiguilleur

On arrive au cœur. `argp` n'agit pas elle-même sur nos données : pour chaque option reconnue, elle appelle **notre** fonction de rappel, et c'est là que tout se décide. Sa signature est imposée par la bibliothèque :

```c
/* NOLINTNEXTLINE(readability-non-const-parameter): argp fixes this signature. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_options *out = state->input;
  switch (key) {
  ...
```

Décortiquons.

**`error_t`** est, dans la glibc, un simple `typedef` pour `int` ; c'est le type de retour conventionnel pour « zéro si tout va bien, un code d'erreur sinon ».

**Les trois paramètres** : `key` est la clé de l'option courante (un `'v'`, un `ARG_ECHO`, ou l'une des clés spéciales qu'on verra) ; `arg` est l'argument associé, s'il y en a un (sinon `NULL`) ; `state` est l'état courant du parsing.

**`state->input`** mérite qu'on s'y arrête longuement, car c'est le point qui prête le plus à confusion. Il faut distinguer **deux canaux qui circulent en sens opposés**.

D'un côté, **`argv`** — la ligne de commande brute — est *l'entrée* : c'est ce qu'argp **lit** et découpe. De l'autre, notre `t_options` est *la sortie* : le réceptacle où l'on **range** le résultat du décodage. Mais argp ne connaît pas notre structure ; pour la lui faire traverser, on la lui confie au lancement comme un pointeur opaque (un `void *`), et elle nous la **restitue intacte** à chaque appel du callback, dans `state->input`. Argp n'y lit ni n'y écrit jamais : elle se contente de la transporter. Le nom « input » est d'ailleurs choisi de *son* point de vue (« une donnée que le programme me fournit ») ; du nôtre, c'est la destination de sortie. Surtout, `argv` ne transite **pas** par `state->input` : ce sont deux paramètres distincts de `argp_parse` (la fonction publique qu'on détaille plus bas), l'un qu'argp consomme, l'autre qu'il nous repasse tel quel.

Le va-et-vient se résume ainsi :

```
options_parse(argc, argv, out)
        argv ───────────►  argp LIT et découpe la ligne de commande
        out  ───────────►  argp la RANGE dans state->input (sans jamais la lire)
                                   │
        pour chaque option trouvée dans argv, argp nous rappelle :
                                   ▼
        parse_opt(key, arg, state)
            key, arg     = ce qu'argp a EXTRAIT de argv  (ex. 'v', ou "5")
            state->input = out   (le même pointeur, rendu tel quel)
                                   │
                                   ▼
        t_options *out = state->input;   /* on récupère notre boîte... */
        out->flags |= OPT_VERBOSE;       /* ...et c'est NOUS qui écrivons dedans */
```

En somme, **argp lit `argv`, mais c'est nous qui remplissons `out`** ; `state->input` n'est que le fil qui nous rend notre boîte à chaque appel, pour qu'on n'ait pas besoin d'une variable globale pour la retrouver. C'est précisément ce qui rend `parse_opt` dépourvu d'état caché — donc réentrant et testable.

Une image, pour fixer les idées : on confie à un trieur (`argp`) une lettre à dépouiller (`argv`) *et* un formulaire vierge (`out`). Le trieur lit la lettre, mais il ne connaît pas nos cases ; alors, pour chaque information qu'il y trouve, il nous rappelle en nous tendant le formulaire et en annonçant « voici : option `v` » — et c'est nous qui cochons la bonne case. `argv` et `out` restent deux objets séparés. Ils ne se toucheront qu'en **un seul point** — les noms d'hôtes —, où `out->hosts` rangera un *pointeur vers l'intérieur de `argv`* plutôt qu'une copie ; on y vient avec `ARGP_KEY_ARG`.

**Le commentaire `NOLINT`.** Notre politique clang-tidy réclame qu'un paramètre pointeur jamais modifié soit déclaré `const`. Or `arg` n'est pas modifié ici — clang-tidy voudrait `const char *arg`. Mais la signature est **dictée par argp** (`char *`, sans `const`) : passer outre casserait la compatibilité de type avec le pointeur de fonction attendu. On neutralise donc l'avertissement, localement et en l'expliquant, sur cette unique ligne.

Le corps est un grand `switch (key)`. Parcourons ses familles de cas.

**Les drapeaux sans argument.** Le cas le plus simple : allumer un bit (vu dans l'article sur `options.h`).

```c
case 'v':
  out->flags |= OPT_VERBOSE;
  break;
```

**Les types de requête.** `--echo`, `--timestamp`, `--address` posent le champ `type` via `decode_type` :

```c
case ARG_ECHO:
  out->type = decode_type("echo");
  break;
```

Comme chaque cas écrase `out->type`, c'est mécaniquement la **dernière** option de type rencontrée qui l'emporte — le comportement « last wins » que les tests vérifient.

**L'aide, l'usage, la version.** Ici, le callback n'imprime rien : il se contente d'**enregistrer l'intention** dans `out->action`, et laisse `main` agir :

```c
case '?':
  out->action = ACT_HELP;
  break;
```

C'est la séparation décrite dans les articles précédents : le décodage *note*, l'appelant *agit*.

**Les options à argument — pour l'instant en attente.** Toutes les options qui prennent une valeur (`-c`, `-i`, `-s`, `--ttl`…) partagent, à cette étape, un traitement minimal :

```c
case 'c':  /* --count */
case 'i':  /* --interval */
...
case ARG_IPTIMESTAMP:  /* --ip-timestamp */
  (void)arg;
  return 0;
```

Le `(void)arg;` mérite un mot : il signale explicitement qu'on *ignore volontairement* l'argument, ce qui évite tout avertissement « variable inutilisée ». Ces options sont donc **reconnues et acceptées** — leur argument est bien consommé par argp —, mais leur valeur n'est pas encore lue ni validée ; ce sera l'objet de l'étape suivante du projet. Les déclarer dès maintenant garantit que l'aide générée est complète et que la ligne de commande est acceptée telle quelle.

**Les opérandes — la clé spéciale `ARGP_KEY_ARG`.** En plus des options, argp transmet au callback chaque **argument non-option** (un nom d'hôte, pour nous) avec la clé spéciale `ARGP_KEY_ARG` (de valeur `0`) :

```c
case ARGP_KEY_ARG:
  if (out->n_hosts == 0) {
    out->hosts = &state->argv[state->next - 1];
  }
  out->n_hosts++;
  break;
```

C'est un idiome subtil. Au moment de cet appel, argp a déjà **avancé** son index interne `state->next` au-delà de l'argument courant ; celui-ci se trouve donc à l'indice `state->next - 1` dans `state->argv`. Plutôt que de **copier** le nom d'hôte, on mémorise, au premier hôte rencontré, l'**adresse** de sa case dans `argv` — `&state->argv[state->next - 1]`. Comme argp regroupe par défaut tous les non-options à la fin du tableau (voir plus bas), les hôtes suivants sont contigus : le couple `hosts` (pointeur de départ) + `n_hosts` (compteur) décrit toute la liste, **sans la moindre allocation**. C'est ce que `ft_ping.h` appelait « une tranche d'`argv` ».

**L'absence d'opérande — `ARGP_KEY_NO_ARGS`.** Si la ligne de commande ne contient *aucun* argument non-option, argp envoie la clé spéciale `ARGP_KEY_NO_ARGS` juste avant la fin :

```c
case ARGP_KEY_NO_ARGS:
  if (out->action == ACT_PING) {
    error_report(state->name, "missing host operand");
    return EX_USAGE;
  }
  break;
```

Mais l'absence d'hôte n'est une erreur que si l'on s'apprêtait à *pinguer* : un `ft_ping --help` n'a évidemment pas besoin d'hôte. D'où le test sur `out->action`. Si c'est bien une erreur, on imprime le diagnostic via notre module `error` (en utilisant `state->name`, le nom du programme qu'argp a initialisé depuis `argv[0]`), puis **on retourne `EX_USAGE`** — la valeur `64`, code de sortie conventionnel pour un mauvais usage. Ce retour non nul stoppe le parsing et remonte jusqu'à `argp_parse`.

**Le cas par défaut.** Toute clé que nous ne traitons pas tombe dans :

```c
default:
  return ARGP_ERR_UNKNOWN;
```

`ARGP_ERR_UNKNOWN` est la constante par laquelle un callback dit à argp « cette clé n'est pas pour moi, débrouille-toi ». C'est un détour amusant : sa valeur réelle est `E2BIG`, un code errno recyclé. Pour les clés que nous ne gérons pas mais qu'argp nous envoie quand même (des notifications d'étape comme « début » ou « fin » du parsing), ce retour est simplement **ignoré** par la bibliothèque — donc inoffensif. Pour une option véritablement inconnue de l'utilisateur, argp prend le relais : elle imprime son propre message et conclut à une erreur.

## Assembler la machine

Table et callback sont réunis dans une `struct argp` :

```c
static struct argp argp = {argp_options, parse_opt, args_doc, doc, NULL, NULL, NULL};
```

Les sept champs sont, dans l'ordre : le tableau d'options, la fonction de rappel, la chaîne `args_doc`, la chaîne `doc`, puis trois facultés avancées qu'on n'utilise pas et qu'on laisse à `NULL` — `children` (composer plusieurs parsers, utile pour des sous-commandes), `help_filter` (réécrire dynamiquement l'aide) et `argp_domain` (traduction `gettext`).

## options_reset : repartir de zéro

Avant chaque analyse, l'état doit être remis aux valeurs par défaut :

```c
static void options_reset(t_options *out) {
  memset(out, 0, sizeof(*out));
  out->action = ACT_PING;
  out->type = PING_ECHO;
  out->count = FT_PING_DEFAULT_COUNT;
  ...
}
```

`memset(out, 0, sizeof(*out))` met d'abord toute la structure à zéro d'un seul geste — ce qui annule `flags`, `n_hosts`, `pattern_len`, etc. On réaffirme ensuite explicitement les défauts (même ceux qui valent zéro, par lisibilité) : le type de requête, les valeurs héritées d'inetutils. C'est cette remise à zéro en tête qui rend `options_parse` **réentrant** — on peut l'appeler en boucle sur le même objet, chaque analyse repart propre. Une propriété précieuse pour les tests, qui enchaînent les cas.

## options_parse : lancer argp sans lui céder le contrôle

La fonction publique principale tient en quelques lignes, mais chacune compte :

```c
int options_parse(int argc, char **argv, t_options *out) {
  error_t rc;
  options_reset(out);
  rc = argp_parse(&argp, argc, argv, ARGP_NO_EXIT | ARGP_NO_HELP, NULL, out);
  if (rc != 0) {
    return EX_USAGE;
  }
  return 0;
}
```

`argp_parse` prend six paramètres : notre `struct argp`, le `argc`/`argv` à analyser, un jeu de **drapeaux**, un pointeur où argp peut écrire l'indice du premier argument non analysé (on passe `NULL` : on n'en a pas besoin, on gère les hôtes via `ARGP_KEY_ARG`), et enfin notre `out` — c'est lui qui deviendra `state->input` dans le callback.

Les deux drapeaux sont le nerf de la guerre.

- **`ARGP_NO_EXIT`** interdit à argp d'appeler `exit()` en cas d'erreur. Sans lui, une option invalide tuerait le processus depuis l'intérieur de la bibliothèque ; avec lui, l'erreur **remonte** sous forme de valeur de retour, et c'est nous qui décidons.
- **`ARGP_NO_HELP`** empêche argp d'**injecter** sa propre option `--help` (et le traitement automatique associé, qui imprimerait l'aide et quitterait). Comme nous voulons maîtriser `--help`, `--usage` et `--version` — les enregistrer comme actions plutôt que de subir un `exit` immédiat —, nous les avons redéclarés dans la table et nous coupons l'automatisme d'argp.

Pourquoi les **deux** ? Parce que la documentation d'argp précise que `argp_parse` peut, elle aussi, appeler `exit` tant que `ARGP_NO_HELP` n'est pas positionné. C'est donc une double garantie : `ARGP_NO_HELP` neutralise la voie « aide », `ARGP_NO_EXIT` neutralise la voie « erreur ». Ensemble, ils assurent que `argp_parse` **revient toujours**. Le code de retour non nul (une option inconnue, ou notre `EX_USAGE` remonté depuis le callback) est alors traduit uniformément en `EX_USAGE` — `main` n'a plus qu'à le transformer en code de sortie.

## options_help et options_usage : imprimer, sans quitter

Reste à produire les textes que `main` affichera :

```c
void options_help(const char *prog) {
  argp_help(&argp, stdout, ARGP_HELP_STD_HELP & ~ARGP_HELP_EXIT_OK, (char *)prog);
}

void options_usage(const char *prog) {
  argp_help(&argp, stdout, ARGP_HELP_USAGE, (char *)prog);
}
```

`argp_help` imprime un message d'aide pour une `struct argp` donnée, sur un flux donné, selon un jeu de drapeaux `ARGP_HELP_*`, en forçant un nom de programme.

`ARGP_HELP_STD_HELP` est la combinaison « réponse standard à `--help` » : elle réunit la ligne d'usage abrégée, la liste longue des options, les deux blocs de documentation (le `doc` coupé par `\v`), l'adresse de bug — **et** un bit, `ARGP_HELP_EXIT_OK`, qui demanderait de quitter avec le code 0. Comme nous voulons seulement **imprimer** et laisser `main` choisir le moment de sortir, nous retirons ce bit par `& ~ARGP_HELP_EXIT_OK` (un « et » binaire avec le complément du masque — la technique d'effacement de bit vue pour `options.h`). En toute rigueur, ce bit n'est honoré que par une fonction sœur destinée à être appelée depuis le parser ; notre masquage est donc surtout **défensif**, mais il dit clairement l'intention : ici, on n'arrête rien.

`options_usage`, lui, ne demande que `ARGP_HELP_USAGE` : la seule ligne `Usage:`, sans la liste détaillée.

Un détail de typage : `argp_help` attend un `char *` pour le nom, non un `const char *`. Notre `prog` étant `const`, on le transtype explicitement par `(char *)prog`. C'est sûr : `argp_help` ne modifie pas ce nom, il ne fait que l'afficher.

## Ce qu'argp fait pour nous, gratuitement

Une grande partie de la valeur d'argp est invisible dans `options.c`, parce qu'elle est *implicite*. Par défaut — c'est-à-dire sans le drapeau `ARGP_IN_ORDER` —, argp **réordonne** la ligne de commande pour regrouper toutes les options en tête et tous les non-options à la fin ; c'est ce qui rend notre « tranche d'`argv` » contiguë et fiable. Elle gère aussi le séparateur `--`, après lequel plus rien n'est interprété comme une option (de sorte qu'un `ft_ping -- -v` traite `-v` comme un nom d'hôte). Et, parce qu'elle repose sur `getopt`, elle accepte les abréviations non ambiguës des options longues. Ces comportements ne nous coûtent pas une ligne : ils viennent avec la bibliothèque, et participent à la fidélité au comportement d'inetutils.

Voilà la mécanique entière : une table qui *décrit*, un aiguilleur qui *trie sans agir au-delà du nécessaire*, et deux fonctions qui lancent la machine et impriment son aide — le tout sans jamais quitter le programme de force, ni posséder la moindre miette de mémoire. La lecture et la validation des valeurs numériques, elles, attendent la page suivante.

## Sources

- `/usr/include/argp.h` (glibc) — les déclarations faisant autorité : `struct argp_option` et `struct argp`, les drapeaux `OPTION_*`, `ARGP_NO_EXIT` / `ARGP_NO_HELP`, les clés `ARGP_KEY_*`, les flags `ARGP_HELP_*`, et les fonctions `argp_parse` / `argp_help`
- `<sysexits.h>` — `EX_USAGE` (valeur 64)
- `<strings.h>` — `strcasecmp` (comparaison insensible à la casse)
- GNU inetutils-2.0, `ping/ping.c` — la table d'options et les formulations reproduites
- Le manuel GNU argp (documentation glibc) — pour les comportements implicites (réordonnancement, `--`, abréviations)
