Pour le modèle d’exécution, je propose de représenter une exécution comme une suite d’états
stabilisés. Chaque transition correspond à un événement RTL : un front actif d’horloge ou, le cas
échéant, l’assertion d’un reset asynchrone.  Avec plusieurs horloges, Lemma considérerait tous les
entrelacements possibles de leurs événements, sans supposer de relation de fréquence ou de phase.
Les entrées resteraient libres, conformément aux sémantiques déjà définies.

le modèle de vérification des assertions. Proposition de départ : une assertion réussit si elle est
respectée dans toutes les exécutions autorisées par le modèle.

les résultats de preuve. Je propose trois issues : PASS (propriété prouvée), FAIL (contre-exemple
trouvé) et UNKNOWN (preuve non concluante, par exemple faute de ressources).

Je propose que tout état soit initialement libre, sauf contrainte explicite dans le RTL ou dans les
hypothèses de vérification. Le reset n’est donc pas supposé actif au démarrage.

le modèle des horloges. Je propose que seules les transitions d’horloge déclarées déclenchent la
logique synchrone, sans supposer de relation de fréquence entre horloges sauf contrainte explicite.

Je propose de ne supposer aucune séquence de reset au démarrage ; sa polarité et ses effets sont
définis par le RTL, et toute séquence attendue doit être ajoutée explicitement aux hypothèses de
vérification.

les entrées: Je propose de les considérer libres à chaque cycle, sauf contraintes explicites du
protocole, comme une règle de stabilité ou un handshake valid/ready.
