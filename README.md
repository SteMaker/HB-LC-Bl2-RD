# HB-LC-Bl2-RD - Selbstbau HomeMatic Gerät für zwei Rolladenaktoren und einen Regensensor - Firmware
## Überblick
Die firmware verwendet die Hardware von https://github.com/SteMaker/HB-LC-Bl2-24VRain-Board/.

**Achtung: Ich übernehme keinerlei Verantwortung für die Sicherheit und Funktionsfähigkeit der Hardware wie auch der Firmware! Die Hardware verwendet Netzspannung 230V ist deshalb lebensgefährlich! Jeder der Inhalte dieses Projekts verwendet tut dies auf eigene Gefahr**

Das Projekt basiert auf der AskSinPP library (asksinpp.de) und verwendet als Basis den 2-fach Rolladenaktor https://github.com/jp112sdl/HM-LC-Bl1-FM-2 von Jérôme (https://github.com/jp112sdl)

Details zur Hardware bitte auf der Seite der Hardware nachschauen.

Der sketch benötigt ein CCU Addon, da es sich um ein von homematic nicht direkt unterstütztes Gerät handelt. Ich arbeite daran das verfügbar zu machen.

Beim ersten Start der firmware wird automatisch eine Direktverknüpfung des Regensensors mit den Rolladen vorgenommen. Sobald Regen detektiert wird, fährt der Rolladen runter / die Markise ein (Richtung OFF). Der Regensensor sollte aber auch für andere Geräte verwendet werden können. Der Status in der CCU funktioniert, ob ein anderer Aktor damit umgehen kann, konnte ich bisher nicht testen.

## Status
Die Firmware ist theoretisch fertig, muss aber noch ein wenig getestet werden. Für das notwendige add-on muss ich erst noch ein Projekt erstellen, bisher ist das praktisch hand gepatched.

## TODOs
- ?? Parameter für den Regensensor, um das event "kein Regen" zu verzögern ??

## Dankeschön
Ein dickes, fettes Dankeschön an die homematic community sowas zu ermöglichen, insbesondere an pa-pa, Jérôme und Marco.
