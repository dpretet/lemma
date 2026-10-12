Les assertions échantillonnent l’état avant le front. Les blocs séquentiels calculent ensuite leurs
prochaines valeurs à partir de ce même état, puis les registres sont mis à jour simultanément.

au front, les assertions lisent l’état courant St​ , puis les flops calculent et mettent à jour
simultanément l’état suivant St+1​.

St​  soit l’état RTL stabilisé juste avant le front d’horloge : la logique combinatoire a fini
de se propager. Au front, les assertions échantillonnent cet état et les blocs séquentiels calculent
leurs prochaines valeurs. Après la mise à jour simultanée des registres, la logique combinatoire se
stabilise pour former St+1​ .  On exclut donc les états transitoires dus à la propagation
combinatoire de la sémantique de preuve

Pour les resets, je propose de distinguer clairement :

Reset synchrone : pris en compte uniquement au front actif de l’horloge.  Reset asynchrone : force
immédiatement les registres à leur valeur de reset, même hors front d’horloge.

C’est important pour la preuve : un reset asynchrone introduit une transition d’état hors des cycles
d’horloge. Je recommande de le prendre en charge explicitement, plutôt que de le ramener
artificiellement à un reset synchrone

Je propose que les registres soient inconnus et non contraints au démarrage, sauf si le RTL définit
une initialisation prise en charge. Un reset ne fixe leur valeur qu’une fois effectivement activé,
selon sa sémantique synchrone ou asynchrone.  Ainsi, Lemma ne supposera pas implicitement qu’un
reset a eu lieu. L’environnement ou les assertions devront préciser si et quand il est activé.


les horloges RTL sont reconnues comme des événements ; Lemma fait avancer chaque domaine d’horloge
de manière abstraite. On ne modélise ni période ni temps physique. Les détails des modules à
plusieurs horloges viendront ensuite. les horloges sont abstraites ; Lemma avance les domaines
d’horloge par événements, sans période ni temps physique.

après cette mise à jour, la logique combinatoire est réévaluée jusqu’à stabilisation avant l’étape
suivante. On retient que la logique combinatoire se stabilise après chaque front, avant l’étape
suivante.

au début de la preuve, les registres non initialisés devraient-ils être considérés comme ayant une
valeur arbitraire ? Cela évite de supposer un reset qui n’est pas garanti par le RTL. Le rapport de
preuve indiquera que les registres sans initialisation explicite commencent avec une valeur
arbitraire. Si cela contribue à un contre-exemple, la trace montrera les valeurs initiales choisies


Les entrées du module sont libres par défaut : le moteur peut leur attribuer n’importe quelle valeur
à chaque cycle. Seules les hypothèses assume les contraignent.

pour la preuve, on peut modéliser les signaux en 2 états (0/1). Les valeurs X et Z ne seraient pas
simulées : leur usage dans le RTL serait signalé comme non pris en charge.
on peut traiter chaque bit X ou Z comme indéterminé : le moteur peut lui attribuer 0 ou 1.

les registres sans reset ni initialisation sont-ils libres au début de la preuve, chacun pouvant valoir 0 ou 1
au début de la preuve, tout FF sans reset ni initialisation a une valeur initiale libre (0 ou 1).

la sémantique des resets.
- reset synchrone : pris en compte uniquement au front actif de l’horloge ;
- reset asynchrone : pris en compte dès son activation, indépendamment de l’horloge.

l’état initial vis-à-vis du reset. Je propose de ne pas supposer implicitement que le reset est
actif au démarrage. Sans contrainte SVA, l’état initial reste libre jusqu’à ce qu’un reset soit
effectivement observé.

reset et front d’horloge simultanés. Je propose de suivre la priorité exprimée par le RTL : un reset
asynchrone actif impose le reset, tandis qu’un reset synchrone est évalué au front d’horloge

je propose de limiter un processus séquentiel à un seul reset, synchrone ou asynchrone. On pourra
traiter les resets multiples plus tard si un besoin concret apparaît.

Je propose de la déduire de la condition RTL (if (rst) ou if (!rst)), sans imposer de polarité par
défaut.

les valeurs affectées par le reset. Je propose d’accepter uniquement des constantes ou des
paramètres résolus, et de refuser les valeurs dépendant d’autres signaux. Cela garde le modèle de
reset simple et prévisible.

chaque registre du processus doit recevoir une valeur explicite dans la branche de reset.
chaque FFD sans reset reçoit aléatoirement 0 ou 1 à l’initialisation, puis conserve cette valeur
jusqu’à sa première mise à jour par l’horloge.

Je propose que les valeurs aléatoires soient tirées une seule fois au démarrage et restent fixes
pendant toute la preuve. Il faut aussi prévoir une seed configurable : même seed, mêmes valeurs
initiales, pour pouvoir reproduire un résultat.

Pour une preuve formelle, une valeur tirée par seed ne couvre qu’un état initial. Pour garantir la
propriété quel que soit l’état initial, chaque FFD sans reset devrait plutôt commencer avec une
valeur arbitraire, mais fixe dans le temps.

Sans reset, un registre démarre à 0 ou 1 ; en preuve, les deux valeurs sont explorées.
Pour les registres avec reset, je propose que le reset ne s’applique qu’en respectant sa sémantique
RTL : asynchrone dès son activation, ou synchrone au prochain front actif. Donc, s’il est actif dès
le démarrage, seul le reset asynchrone initialise immédiatement le registre.

désactivation du reset
Je propose qu’une fois le reset désactivé, le registre garde sa valeur. Il ne reprend le chargement
de ses données qu’au prochain front actif de l’horloge.  C’est vrai pour un reset synchrone comme
asynchrone.

Pour chaque registre avec reset,  chaque registre peut avoir sa propre valeur de reset, 0 ou 1.
On met ce que le user a ecrit dnas le code comme valeur de reset

priorité du reset. Si le RTL combine reset et enable, on suit strictement l’ordre et la priorité
exprimés dans le RTL, sans règle supplémentaire

dans un bloc séquentiel, si aucun branchement n’assigne de valeur à un registre lors d’un front
actif, il conserve sa valeur précédente. On suit bien cette sémantique du RTL

Nous avons décidé qu’un bit X ou Z peut valoir 0 ou 1. Il reste à préciser quand ce choix peut changer.

Je propose la règle suivante : le choix est fait à chaque évaluation. Ainsi, une expression combinatoire contenant X peut produire une valeur différente d’un cycle à l’autre. Si cette valeur est capturée dans un registre, le registre la conserve jusqu’à sa prochaine mise à jour.

Reset asynchrone et front d’horloge simultanés
Je propose que si le reset asynchrone est actif au front d’horloge, le reset soit prioritaire et que le registre prenne sa valeur de reset.

Plusieurs registres au même front
Je propose que tous les registres déclenchés par le même front lisent les valeurs avant ce front, puis mettent à jour leurs sorties ensemble. Cela correspond à la sémantique des affectations non bloquantes <=

 Fronts simultanés sur des horloges différentes
Je propose que si plusieurs horloges ont un front au même instant, tous les registres concernés
échantillonnent l’état précédent, puis mettent à jour leurs sorties ensemble.

Stabilisation de la logique combinatoire
Après la mise à jour des registres, la logique combinatoire est réévaluée jusqu’à stabilisation
avant l’évaluation des propriétés SVA.

Petite précision importante : les assertions SVA concurrentes échantillonnent les signaux au
déclenchement de l’horloge, avant ces mises à jour, puis évaluent la propriété à partir des valeurs
échantillonnées. Il ne faut donc pas les évaluer simplement sur l’état stabilisé après le front

On prend en charge disable iff dans les assertions SVA
