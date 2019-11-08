# HB-LC-Bl2-RD - Selbstbau HomeMatic Gerät für zwei Rolladenaktoren und einen Regensensor - Firmware
## Überblick
Die firmware verwendet die Hardware von <to be uploaded>.

**Achtung: Ich übernehme keinerlei Verantwortung für die Sicherheit und Funktionsfähigkeit der Hardware wie auch der Firmware! Die Hardware verwendet Netzspannung 230V ist deshalb lebensgefährlich!**

Die Entwicklung befindet sich in einem frühen Stadium, ich muss erst noch alle Teile auf github hochladen.

Details zur Hardware bitte auf der Seite der Hardware nachschauen.

Der sketch benötigt ein CCU Addon, da es sich um ein von homematic nicht direkt unterstütztes Gerät handelt. Ich arbeite daran das verfügbar zu machen.

Um den Regensensor mit dem Rolladenaktor zu verknüpfen - z.B. um eine Markise bei Regen einzufahren - muss eine Direktverknüpfung mit dem homematic-manager ersstellt werden. Das CCU Webui unterstüzt keine Direktverknüpfungen innerhalb eines Gerätes.

## TODOs
- Die eigentlich Regensensorfunktionalität fehlt noch. Zur Zeit wird alle 5 Minuten zwischen Regen und kein Regen umgeschaltet
- Direktverknüpfung zwischen Regensensor und den zwei Aktoren beim Start automatisch anlegen
- ?? Parameter für den Regensensor, um das event "kein Regen" zu verzögern ??
