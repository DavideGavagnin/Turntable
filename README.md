# An ESP8266 (WemosD1 mini) that adds some automatism to a manual turntable

Questo è un circuito atto ad aggiungere qualche automatismo ad un giradischi manuale.

Alla versione attuale del sistema, il braccio del giradischi si alza alla fine del disco, il giradischi viene fermato e il circuito si spegne.
E' presente anche un webserver che risponde a comandi REST di pause/stop/play(45|33)/status in GET.

![Proto](resources/IMG_20240502_120353859.jpg?raw=true "Proto")
(foto v0.0.1)

Il fine disco viene intercettato tramite interruzione del fascio ottico del sensore collegato in D2. Quando ciò accade, viene attivato il servocomando che
alza il braccio dal disco, quindi viene attivato il servocomando che spegne la rotazione del giradischi tramite il selettore di velocità.
Infine, il relè di alimentazione viene disattivato, spegnendo il circuito.

Per accendere il circuito occorre intervenire manualmente sul pulsante. Tenendolo premuto qualche istante, l'alimentazione al microcontrollore viene ripristinata "bypassando" 
il relè; bastano pochi istanti che il microcontrollore arrivi ad attivare l'uscita D0 per mantenere il relè eccitato, a quel punto il pulsante non ha alcuna funzione,
 e il circuito rimane acceso.

L'attivazione del relè della velocità (accensione del giradischi) avviene tramite API. Aggiungerò altri due pulsanti di input per l'accensione con selezione della velocità.

## Release notes

v1.0.0: Spostato il servocomando del pomello della velocità, ora può selezionare 33 o 45 giri oltre a spegnere il giradischi, migliorata la risposta alle richieste via API.
v0.2.0: Aggiunto circuito con Relè shield e push-button per spegnere il circuito dopo lo stop del giradischi
v0.1.1: Webserver per attivare il comando di pausa e stop via web request    
v0.1.0: Aggiunto servo per spostare la levetta da 33 giri a off    
v0.0.1: Prima release