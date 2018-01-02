Projet Commande Electrovalve de Spray
======================================
### Description:

Ce projet est basé sur la programmation en language C de microcontrôleur Microchip [PIC16F1825][1].

Ce montage commande une Electrovalve servant à activer le spray sur un fauteuil dentaire.
Deux micro-switch BP et PEDAL sont relus par le microcontrôleur et détectent respectivement une demande de mise en service du Spray, et un appui sur la pédale de mise en route des instruments.

![Alt text](/Schema_BT.jpg)

Il existe 4 modes de fonctionnement:
Mode 0 : Normal avec interlocking : Une impulsion sur BP sans PEDAL  active/désactive SPRAY
Mode 1 : Normal sans interlocking : Une impulsion sur BP quel que soit l’état de PEDAL  active/désactive SPRAY
Mode 2 : Old fashion way  i.e. SPRAY actif tant que BP appuyé
Mode 3 : Backup - SPRAY toujours actif

Dans tous les cas, il faut au moins avoir PEDAL pour avoir EV d’où : EV = PEDAL.
Le mode de fonctionnement est défini en fonction des relectures sur CONFIG_1 et CONFIG_2.

Pour la partie Hardware, on a un schéma à 2 étages :
	1er étage : relais miniature qui bascule en fonction de l’état de BP :
		- 1 contact qui commande le 2eme étage. 
		- 1 contact sec utilisé pour l’affichage led au module central
	2ème  étage : transistor qui commande l’Electrovalve + LED de signalisation en fonction de l’état de EV.

De plus, une LED de signal de vie se contente de clignoter lorsque le montage est sous tension.

### Prérequis:

Ce projet utilise les outils suivants:
 * [Microchip MPLAB X][2] (v2.15 ou plus récent)
 * [Microchip XC8 Compiler][3] (v1.44 ou plus récent)
 * [DesignSpark PCB][4] (v8.0 ou plus récent)
 
 
[1]: http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en546902 "PIC 16F1825"
[2]: http://www.microchip.com/pagehandler/en-us/family/mplabx/ "MPLAB X"
[3]: http://www.microchip.com/pagehandler/en_us/devtools/mplabxc/ "MPLAB XC Compilers"
[4]: http://www.rs-online.com/designspark/electronics/eng/page/designspark-pcb-home-page "DesignSpark PCB"
