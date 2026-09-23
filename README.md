# NebiPad+

Editor di testo desktop in C++17 e Qt 6. Questa base include schede, numeri di riga, apertura e salvataggio dei file, stampa, comandi di modifica, ricerca e sostituzione, e avviso per modifiche non salvate. Ogni linguetta ha un mini floppy a sinistra del nome per attivare la sola lettura e, prima del pulsante di chiusura, un pin che protegge la scheda e la mantiene nel gruppo iniziale. La toolbar usa icone vettoriali originali incluse nel progetto. Il ritorno a capo automatico è attivo all'avvio; lo zoom funziona dai pulsanti o con Ctrl/⌘ + rotellina. La barra di stato mostra tipo di file, lunghezza, righe, posizione del cursore, sistema operativo e codifica UTF-8.

In NebiPad+, **Strumenti → Generatore SHA-256 / SHA-512** calcola hash esadecimali da testo UTF-8 o da file. I file vengono letti a blocchi, quindi è possibile elaborare anche contenuti di grandi dimensioni senza caricarli interamente in memoria. Il risultato può essere copiato negli appunti.

**Strumenti → OpenPGP** e i pulsanti lucchetto della toolbar crittografano il documento attivo per una chiave pubblica e lo decrittografano solo quando la chiave privata corrispondente è disponibile. Dal medesimo menu si possono creare una coppia di chiavi protetta da password e importare le chiavi pubbliche dei destinatari; se il portachiavi è vuoto, la cifratura propone direttamente entrambe le operazioni. NebiPad+ usa GnuPG e produce messaggi ASCII Armor compatibili tra macOS, Windows e Linux. Cerca automaticamente Homebrew/MacGPG, Gpg4win e le installazioni Linux standard; è possibile scegliere manualmente un eseguibile `gpg`. Se il programma `pinentry` non può aprire la richiesta grafica, NebiPad+ mostra una propria finestra protetta e passa la password a GnuPG tramite standard input. Le password non vengono inserite nella riga di comando né memorizzate dall'applicazione. Se un'operazione fallisce, il contenuto originale del documento non viene sostituito.

**Strumenti → Generatore password casuali** crea da una a venti password, con lunghezza da 8 a 128 caratteri. Puoi includere minuscole, maiuscole, numeri e simboli ed escludere `I`, `l`, `1`, `O`, `0`, `o` e `|`. Ogni password contiene almeno un carattere di ciascun gruppo selezionato. La finestra mostra robustezza ed entropia stimate e consente di copiare i risultati. La generazione usa `QRandomGenerator::system()`, che Qt documenta come sorgente casuale di sistema crittograficamente sicura su macOS, Windows e Linux.

All'avvio compare per 6 secondi uno splash screen con l'immagine originale di NebiPad+. Il marchio è disponibile in `assets/nebipad-splash.svg` e `assets/nebipad-logo.svg`, con esportazioni PNG nelle stesse cartelle. Le risorse SVG sono integrate nell'eseguibile tramite Qt e vengono usate anche come icona della finestra. L'icona applicazione usa lo stesso logo: `assets/NebiPad.icns` per macOS, `assets/NebiPad.ico` per Windows e il PNG con file `.desktop` per Linux.

Dopo lo splash compare la schermata iniziale con i quattro pulsanti **NebiPad+**, **MindNet**, **Impagination** e **Math**. La sezione **File recenti** è condivisa da tutta la suite, si aggiorna mentre la schermata rimane aperta e mostra accanto a ogni nome il tipo di applicazione tra parentesi. Selezionando un recente viene avviata automaticamente l'applicazione corretta. Sono conservati fino a dieci documenti aperti o salvati; i percorsi non più validi vengono rimossi. Il pannello sinistro usa il marchio verticale `assets/nebipad-welcome.svg`.

La schermata iniziale rimane aperta quando si avvia una delle quattro applicazioni, si crea un documento o si apre un file. Si possono quindi avviare altri spazi di lavoro dalla stessa schermata.

Dopo aver scelto un'applicazione compare per 4 secondi il relativo splash screen. NebiPad+ usa il marchio originale; MindNet, Impagination e Math usano lo stesso logo con il proprio nome e uno sfondo rispettivamente verde petrolio, indaco e viola. Le varianti SVG e PNG sono in `assets/nebipad-splash-*.svg` e `assets/nebipad-splash-*.png`.

La schermata iniziale offre anche quattro pulsanti per la suite. **NebiPad+** apre l'editor di codice esistente. **MindNet** apre una tela A4 o A3 con forme spostabili, etichette modificabili, collegamenti e zoom (anche con Ctrl+rotella); salva diagrammi in JSON con estensione `.mindnet.json`. **Impagination** apre una pagina A4 o A3 per documenti formattati, salvati in HTML. **Math** apre una pagina A4 per testo e simboli matematici Unicode, salvata in HTML. I tre spazi dedicati non mostrano il menu Linguaggio e non attivano completamento o colorazione del codice. Le icone della selezione sono in `assets/suite-*.svg`.

Impagination offre stili Normale/Titolo/Sottotitolo/Titolo 1–3, dimensione del carattere, formattazione, evidenziazione, apice e pedice, maiuscole/minuscole, direzione del paragrafo, allineamento, elenchi puntati e numerati, rientri, interlinea e spazio dopo il paragrafo. Il menu **Inserisci** aggiunge tabelle, immagini, collegamenti, simboli, data e ora, righe orizzontali e interruzioni di pagina; **Strumenti → Tabella** aggiunge o rimuove righe/colonne e unisce o divide celle. Sono presenti ricerca, sostituzione, conteggio parole, formato A4/A3, orientamento, margini e esportazione PDF. Il salvataggio HTML include le immagini e conserva le impostazioni della pagina in un commento leggibile anche da altri editor HTML. L'esportazione PDF usa il formato e l'orientamento correnti. Il formato DOCX/ODT e la revisione collaborativa non sono ancora supportati.

Impagination apre anche documenti PDF tramite Qt PDF. Ogni pagina viene importata ad alta risoluzione, proporzionata alla pagina del documento e separata con un'interruzione di pagina. Il PDF originale non viene sovrascritto: al primo salvataggio viene proposto un nuovo documento HTML modificabile. I PDF protetti da password richiedono prima la rimozione della protezione.

Il menu **PDF** di Impagination integra PoDoFo per modificare direttamente i file PDF. Offre apertura e creazione, aggiunta di testo e immagini con pagina e coordinate, inserimento ed eliminazione di pagine A4, rotazione, riordinamento, unione di un secondo PDF e modifica di titolo, autore, oggetto e parole chiave. Ogni scrittura viene prima completata in un file temporaneo; l'originale viene sostituito solo dopo un salvataggio riuscito e il documento viene ricaricato automaticamente. Qt PDF continua a occuparsi dell'anteprima, mentre PoDoFo gestisce la struttura e i contenuti PDF.

Impagination include la stessa dettatura vocale di NebiPad+: si attiva dal menu **Strumenti → Dettatura vocale**, dal pulsante microfono della toolbar oppure con **F12**. L'equalizzatore blu e viola appare in basso durante l'ascolto, mostra la trascrizione parziale e inserisce il risultato nella posizione del cursore mantenendo la formattazione corrente.

La toolbar di Impagination include anche **Registra nota audio**, **Play/Pausa**, **Stop** e il controllo del guadagno. Durante la registrazione usa lo stesso equalizzatore della dettatura e inserisce nel testo marcatori compatti come `[Registrazione_1]`; i WAV e l'indice dei collegamenti vengono conservati nella cartella `recording` accanto al documento e copiati con Salva con nome. Con Ctrl/⌘+clic sul marcatore si riproduce la traccia. Le linguette di Impagination hanno inoltre il mini floppy per la sola lettura e il pin prima del pulsante di chiusura; le schede fissate restano a sinistra e sono protette dalla chiusura accidentale.

Il menu **Riferimenti** di Impagination inserisce un indice basato sui paragrafi Titolo 1–3 e permette di aggiornarlo dopo le modifiche. Aggiunge segnalibri nominati, navigazione ai segnalibri e note finali numerate con collegamenti al testo. Tieni premuto Ctrl o ⌘ e clicca un collegamento interno per raggiungere la relativa voce. **Strumenti → Tabella** offre anche colore della cella e spessore dei bordi. In Modifica sono disponibili Trova successivo (F3) e Trova precedente (Maiusc+F3). Indice, segnalibri e note sono conservati nel file HTML.

Per modificare un'immagine inserita in Impagination, fai doppio clic su di essa, usa il tasto destro oppure scegli **Strumenti → Immagine**. Nell'editor immagine le maniglie agli angoli cambiano larghezza e altezza e la maniglia superiore regola la rotazione. **Mantieni proporzioni** è attivo inizialmente e collega larghezza e altezza sia nei campi numerici sia nel trascinamento delle maniglie; puoi disattivarlo per cambiare le dimensioni separatamente. Il pulsante **Ritaglia** permette di trascinare l'area da conservare; premi **Applica ritaglio** e poi **OK** per sostituire l'immagine nel documento. Sono disponibili anche i valori numerici di larghezza, altezza e angolo. L'immagine elaborata resta incorporata nel file HTML. Dal menu destro **Contorno testo** puoi disporre l'immagine a sinistra o a destra: il testo riempie lo spazio libero riga per riga per tutta l'altezza dell'immagine e prosegue a pagina piena sotto di essa. Puoi anche riportarla in linea. Con **Sposta** nel menu destro, trascina l'immagine in un altro punto della pagina; la posizione viene salvata nel documento.

**Inserisci → Video** aggiunge video dal PC o da YouTube come schede nella pagina. Fai doppio clic sulla scheda per riprodurre il video nell'applicazione. I file locali vengono copiati nella cartella `<nome documento>_media` accanto al file HTML. I video YouTube richiedono Qt WebEngine e una connessione Internet; nel lettore è disponibile anche **Apri su YouTube** se il player integrato non carica il video. Se WebEngine non è disponibile, il video si apre nel browser.

**Inserisci → Grafico** crea grafici a colonne, barre, linee, area o torta. La finestra dati consente di modificare categorie e serie, aggiungere o rimuovere righe e colonne, scegliere titolo, legenda e tema dei colori. Il grafico viene adattato automaticamente alla larghezza utile della pagina A4/A3 e generato ad alta risoluzione. Con un clic sul grafico compaiono quattro maniglie agli angoli: trascinale per ingrandirlo o ridurlo mantenendo le proporzioni. Rimangono disponibili anche ritaglio, rotazione, spostamento e disposizione rispetto al testo.

Su macOS, dopo la compilazione usa `scripts/package_macos.sh build-release/NebiPadPlus.app` per distribuire Qt e firmare il processo WebEngine con le autorizzazioni richieste dal player.

In **Layout → Intestazione e piè di pagina** puoi attivare separatamente intestazione, piè di pagina e numeri di pagina. I testi sono modificabili sul foglio e le impostazioni vengono salvate nel file HTML; stampa ed esportazione PDF li ripetono su ogni pagina.

In MindNet la palette a destra permette di scegliere **Diagrammi di flusso** o **Networking**. La prima categoria contiene processo, inizio/fine, decisione, input/output, documento, database, connettore, sottoprocesso e attesa. Networking offre 22 simboli vettoriali, tra cui router, switch Layer 2/3, PC, laptop, Wi-Fi, access point, server, firewall, cloud, NAS, dispositivi mobili e apparati di rete. Tutti i simboli si possono spostare, ridimensionare, ruotare e salvare. Si può scegliere il colore delle forme, anche dopo averle selezionate. Il pulsante con l'icona del collegamento offre linee continue o tratteggiate, con o senza freccia, con spessore e colore regolabili. Per creare una linea, scegli lo stile e trascina sul foglio dal punto iniziale a quello finale; non occorre selezionare figure. Con lo strumento Linea attivo, i punti di ancoraggio appaiono sui vertici e sui lati delle figure e sui vertici delle linee esistenti: avvicina il cursore per unire esattamente le estremità. Lo strumento resta attivo per disegnare altre linee; premi Esc o l'icona Seleziona per tornare alla selezione. Il bordo del foglio mostra la fine dell'A4/A3. Intestazione e piè di pagina si modificano dalla palette e sono salvati nel file del diagramma.

Le barre degli strumenti della suite mostrano pulsanti con icone. In MindNet il menu **Linee** offre **Linea** per un collegamento continuo e le altre varianti tratteggiate e con freccia; il pulsante della barra mostra un'anteprima della linea e apre gli stessi stili.

Il pulsante **Casella di testo** di MindNet aggiunge sul foglio un riquadro spostabile in cui si può scrivere direttamente. La barra **Testo MindNet** offre font, dimensione numerica, pulsanti A+ e A− per aumentare o diminuire il carattere, grassetto, corsivo, sottolineato, barrato e colore. I controlli funzionano anche selezionando direttamente una figura dei diagrammi di flusso o di Networking: modificano il suo nome. La formattazione e il contenuto delle caselle e dei nomi sono conservati nei file `.mindnet.json`.

Il menu **Diagramma → Inserisci diagramma** contiene modelli modificabili per flusso, mappa mentale, organigramma, albero decisionale, processo, rete, UML e blocchi. Nella barra principale di MindNet il formato A4/A3 si combina con il pulsante per orientamento verticale/orizzontale. I pulsanti **Righelli** e **Griglia** mostrano o nascondono gli aiuti sul foglio. La barra **Misure MindNet** permette di impostare separatamente passo del righello e passo della griglia in mm, cm o px; i valori e le opzioni sono salvati in ogni documento MindNet.

Selezionando una figura compaiono quattro maniglie quadrate per ridimensionarla e una maniglia sopra il bordo per ruotarla. Trascinando il nome della figura si spostano insieme figura e testo; Alt+trascinamento sposta solo il nome, mentre il doppio clic permette di modificarlo. L'etichetta può essere selezionata e trascinata anche dal bordo: otto maniglie ne regolano le dimensioni e quella superiore la rotazione. Le linee selezionate mostrano una maniglia per ogni vertice: trascinala per modificare il percorso, oppure trascina il segmento per spostarlo. Il tasto destro su forme, simboli, testi e linee apre le opzioni contestuali, tra cui **Elimina oggetto**; per le linee sono disponibili anche aggiunta/rimozione dei vertici, stile, colore e spessore. Dimensioni, rotazione e vertici sono conservati nei file MindNet; i diagrammi salvati nelle versioni precedenti restano apribili.

La seconda riga della toolbar offre scelta del carattere, grassetto, corsivo, sottolineato, barrato, colore e allineamento. I documenti HTML conservano la formattazione. Se si salva un documento formattato come testo semplice, l'app propone di salvarlo in HTML oppure di perdere la formattazione.

La barra dei menu comprende File, Modifica, Ricerca, Visualizza, Formato, Linguaggio, Configurazione, Strumenti, Macro, Esegui, Plugin, Finestra e ?. I menu ancora senza funzioni sono predisposti per le prossime funzionalità.

Nel menu Formato, **Codifica** rilegge i byte del file nella codifica scelta; per le voci con BOM il file deve contenere quel BOM. **Converti in** mantiene il testo visibile e usa la nuova codifica al successivo salvataggio. Sono disponibili ANSI (Windows-1252), UTF-8, UTF-8-BOM, UTF-16 BE BOM e UTF-16 LE BOM. La codifica è mostrata nella barra di stato; i caratteri non rappresentabili in ANSI impediscono il salvataggio in quella codifica.

Il menu Finestra ordina le schede per nome, percorso, tipo, dimensione o data di modifica in entrambi i versi. La scheda attiva rimane selezionata. Per i documenti modificati, la dimensione usata nell'ordinamento è quella del contenuto corrente; per gli altri file è la dimensione su disco.

Il menu Linguaggio contiene 114 linguaggi e formati, raggruppati in sottomenu per lettera iniziale e ordinati alfabeticamente. Include i linguaggi integrati di Notepad++ e altri linguaggi diffusi. La scelta attiva l'evidenziazione della sintassi e l'indentazione automatica, e appare nella barra di stato al posto di “Normal Text File”. L'estensione e lo shebang riconoscono il linguaggio all'apertura. Quando si cambia linguaggio manualmente, lo shebang in prima riga viene sostituito con quello nuovo o rimosso se il linguaggio non lo usa. HTML e CSS sono modificati come codice sorgente.

## Completamento del codice

La riga del cursore è evidenziata in verde tenue. Dopo due caratteri, NebiPad+ propone parole chiave del linguaggio, simboli standard comuni e nomi presenti nel documento. `Ctrl+Spazio` apre i suggerimenti manualmente; `Invio` o `Tab` inseriscono quello selezionato, `Esc` chiude il pannello. Per le funzioni riconosciute nel documento vengono inserite le parentesi, con il cursore al loro interno.

Per suggerimenti semantici, NebiPad+ usa il Language Server Protocol (LSP). Cerca automaticamente nel `PATH` il server installato per C/C++/Objective-C (`clangd`), Swift (`sourcekit-lsp`), Python (`basedpyright-langserver`, `pyright-langserver`, `pylsp` o `jedi-language-server`), JavaScript/TypeScript (`typescript-language-server`), Rust (`rust-analyzer`), Go (`gopls`), Lua (`lua-language-server`), shell (`bash-language-server`), HTML/CSS/JSON (server VS Code dedicati) e YAML (`yaml-language-server`). Su macOS cerca anche in `/opt/homebrew/bin` e `/usr/local/bin`. Su Windows, Linux e macOS, se il server non è presente, il completamento integrato continua a funzionare per tutti i linguaggi del catalogo.

## Dettatura vocale

Premi **F12** nell'editor per iniziare a parlare e premi di nuovo **F12** per terminare. Il testo riconosciuto viene inserito alla posizione del cursore nel documento selezionato. Un indicatore nella parte inferiore centrale mostra il livello delle frequenze del microfono, dal blu al viola. Su macOS mostra anche la trascrizione provvisoria mentre parli. La prima volta macOS chiede il permesso per microfono e riconoscimento vocale.

Su Windows e Linux, la dettatura usa `whisper-cli` e un modello Whisper multilingue in formato `.bin`. Installa `whisper-cli`, poi scegli l'eseguibile e il modello dal menu **Strumenti**; al termine della registrazione viene eseguita la trascrizione locale. Puoi anche impostare `NEBIPAD_WHISPER_CLI` e `NEBIPAD_WHISPER_MODEL`. Il modello non è incluso nell'app. La scorciatoia F12 può richiedere anche il tasto Fn, a seconda della tastiera.

## Registrazioni audio nel testo

I tre pulsanti audio nella toolbar sono separati dalla dettatura. **Registra voce** avvia una registrazione WAV e inserisce nel punto del cursore un'etichetta breve come `[Registrazione_1]`, `[Registrazione_2]` e così via. Il nome completo del WAV resta nella cartella `recording`; l'associazione fra etichetta e file viene salvata in un indice nella stessa cartella. **Stop** termina la registrazione e chiude il file; **Play / Pausa** riproduce o mette in pausa il riferimento più vicino al cursore. Puoi anche usare **Ctrl+clic** o **⌘+clic** sull'etichetta. L'equalizzatore blu‑viola della dettatura compare anche durante la registrazione.

Il controllo percentuale accanto ai pulsanti audio regola il livello delle nuove registrazioni (75% all'avvio, impostazione salvata). Se il microfono distorce, abbassalo; i WAV già registrati non vengono modificati.

I documenti con i vecchi riferimenti `[Audio: ...](...)` vengono convertiti alle etichette brevi quando vengono riaperti. Il documento viene segnato come modificato per permettere di salvare la conversione; i WAV non vengono rinominati.

Se il documento è nuovo, NebiPad+ chiede prima di salvarlo. I WAV vengono conservati nella cartella `recording` accanto al documento; con **Salva con nome** in un'altra cartella, NebiPad+ copia le registrazioni collegate nella nuova cartella `recording`. Se la cartella del documento è di sola lettura, l'app salva il WAV nella propria cartella dati `recording` e inserisce nel testo il percorso completo del file.

## Requisiti

- CMake 3.21 o successivo
- Qt 6.2 o successivo, moduli Widgets, Svg, PrintSupport e Multimedia
- Compilatore C++17

Su macOS con Homebrew: `brew install cmake qt`

## Compilazione

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
```

Su sistemi in cui Qt 6 è già visibile a CMake, omettere `-DCMAKE_PREFIX_PATH`.

Su macOS l'app si trova in `build/NebiPadPlus.app`.

## Build automatiche e installer

Il workflow `.github/workflows/build-installers.yml` compila e prova la suite su macOS,
Windows e Linux. Ogni esecuzione rende disponibili gli artifact DMG, installer EXE,
AppImage e DEB. Creando un tag come `v0.1.0`, gli stessi file vengono pubblicati
automaticamente in una GitHub Release.

Le directory di build, le registrazioni, i documenti temporanei, le credenziali OAuth
e i token Google Drive sono esclusi dal repository. File recenti, lingua, macro e
account cloud rimangono nelle preferenze locali di ciascun utente.
