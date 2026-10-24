# Feuille de route

Ce qui manque au pilote MX870, au 24 octobre 2026, après les premiers essais sur une
imprimante réelle (firmware 1.030, encres compatibles, papier ordinaire A4). Le pilote CUPS
installé avec `install.sh --printer` imprime correctement depuis les applications. Les constats
détaillés sont au §5 de [docs/protocole-mx870.md](docs/protocole-mx870.md).

Rappel de méthode : la référence est le pilote Windows 1.06 et le comportement de
l'imprimante ; Gutenprint n'est qu'un indice. L'objectif est une impression correcte au
quotidien, pas une colorimétrie parfaite.

## À valider sur l'imprimante (code écrit, jamais essayé)

| Fonction | Où | Comment valider |
|---|---|---|
| Application AirPrint (PAPPL) | `src/app/mx870-app.c`, `install.sh --app` | ajouter l'imprimante `bjnp://IP`, imprimer depuis macOS |
| Recto-verso (bord long, bord court) | `ESC ($`, `ESC (s`, `ESC (u` dans `src/libmx870/encoder.c` | `sample_a4_plain_duplex.bin` puis un document de 2 pages |
| Ordre de sortie inversé par défaut (sortie face vers le haut) | PPD `*DefaultOutputOrder: Reverse`, PAPPL `output_face_up` | imprimer 3 pages, ordre « Automatique » dans le dialogue |
| Choix du bac (arrière, cassette) | `ESC (l` | une page par bac |
| Sans bordure | marges négatives, `ESC (p` | photo 10 × 15 sur papier photo |
| USB (application AirPrint) | PAPPL | brancher en USB, imprimer |
| USB (pilote CUPS) | backend `usb://` de macOS avec notre PPD | non prévu : à essayer avant de le documenter |

## Qualité d'impression

- **Papiers photo** : modes 4 bits (C/M 6 ou 8 niveaux) jamais essayés ; sur papier ordinaire,
  l'enregistrement 6 niveaux produisait des lignes blanches au pas du bloc de 16 lignes.
  Pas de correction des couleurs pour ces supports.
- **Mode 7** (Photo Paper Pro II, qualité 4) exclu : plans supplémentaires `C|0x80`… non
  décodés ; `cnmop9z.dll` leur applique les tables de `0x66526088`.
- **Qualité haute en niveaux de gris** : seul un enregistrement 6 niveaux existe (`0x31`) ;
  elle imprime pour l'instant comme la qualité standard.
- **Règle de sélection Windows** entre les deux enregistrements « papier ordinaire, qualité
  haute » de `cnm_0367.dll` : non lue (base de données à interface générique).
- **Très légères lignes sur le noir** en qualité haute ; quelques lignes isolées en brouillon.
- **Couleurs** : correction réglée à l'œil (papier ordinaire, encres compatibles), un peu
  terne. Les tables d'étalonnage de `cnmdump5.dll` ne sont pas exploitées.
- **Niveaux de gris** : pas de courbe de correction (la couleur en a une).

## Fonctions du pilote Windows absentes

- **État de l'imprimante** (bourrage, papier, capot, erreurs) et **niveaux d'encre** :
  Language Monitor `cnmlmon2.dll`, commandes XML IVEC `GetStatus`, lecture BJNP `0x20`.
  Aujourd'hui l'application AirPrint ne remonte aucun état (`bjnp_status`).
- **Entretien depuis le Mac** : vérification des buses, nettoyage, nettoyage en profondeur,
  alignement des têtes (faisables depuis le panneau de l'imprimante).
- **Réglages avancés** de `cnmdump5.dll` (§3 du document de protocole) : balance des couleurs,
  densité, contraste, luminosité, mode monochrome teinté, impression silencieuse.

## Outils et confort

- Message d'erreur BJNP : affiche toujours le port 8611, même pour `HOST:PORT`.
- `mx870-testpage` : la mire `colors` applique ses corrections par-dessus celle de
  l'encodeur ; à revoir si l'étalonnage reprend.

## Hors périmètre

- Scanner et fax (la MX870 est multifonction) : relèvent d'autres pilotes.
