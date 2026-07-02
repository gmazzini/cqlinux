<html>
<pre>
cd /home/gmazzini/gm2
sh cqlinux/go
php cqlinux/ts890s_on.php
ping 10.0.0.10
</pre>

<h2>cqrun WHOIS interface</h2>

<p>
cqrun espone una piccola interfaccia TCP testuale sulla porta <code>4343</code>,
utile per leggere stato, decode e diagnostica da remoto.
Tutti i comandi richiedono la chiave <code>KEY</code>.
</p>

<h3>Uso</h3>

<pre><code>printf "KEY status\r\n" | nc 10.0.0.8 4343
printf "KEY version\r\n" | nc 10.0.0.8 4343
printf "KEY cqed\r\n" | nc 10.0.0.8 4343
printf "KEY rxed\r\n" | nc 10.0.0.8 4343
printf "KEY freefreq\r\n" | nc 10.0.0.8 4343</code></pre>

<p>
I comandi di controllo usano la forma storica:
</p>

<pre><code>printf "set KEY ft8\r\n" | nc 10.0.0.8 4343
printf "set KEY ft4\r\n" | nc 10.0.0.8 4343
printf "set KEY even\r\n" | nc 10.0.0.8 4343
printf "set KEY odd\r\n" | nc 10.0.0.8 4343
printf "set KEY 1500\r\n" | nc 10.0.0.8 4343</code></pre>

<h3>Comandi</h3>

<table>
<tr><th>Comando</th><th>Descrizione</th></tr>
<tr><td><code>KEY help</code></td><td>Mostra i comandi disponibili</td></tr>
<tr><td><code>KEY version</code></td><td>Release cqrun, data compilazione, versione WSJT-X e uptime</td></tr>
<tr><td><code>KEY heartbeat</code></td><td>Età dell'ultimo heartbeat WSJT-X</td></tr>
<tr><td><code>KEY status</code></td><td>Frequenza, modo, Enable TX, RX DF e TX DF</td></tr>
<tr><td><code>KEY rxed</code></td><td>Decode ricevuti da WSJT-X</td></tr>
<tr><td><code>KEY cqed</code></td><td>Diagnostica della selezione CQ</td></tr>
<tr><td><code>KEY freefreq</code></td><td>Intervalli audio liberi nella finestra 200-3000 Hz</td></tr>
<tr><td><code>KEY used</code></td><td>Call già selezionati/usati</td></tr>
<tr><td><code>KEY logged</code></td><td>QSO già presenti nel log ADIF</td></tr>
<tr><td><code>KEY escluded</code></td><td>Call in blacklist; alias: <code>excluded</code></td></tr>
<tr><td><code>KEY read N</code></td><td>Legge un valore interno numerico</td></tr>
</table>

<h3>Comandi di controllo</h3>

<table>
<tr><th>Comando</th><th>Azione</th></tr>
<tr><td><code>set KEY ft8</code></td><td>Imposta WSJT-X in FT8</td></tr>
<tr><td><code>set KEY ft4</code></td><td>Imposta WSJT-X in FT4</td></tr>
<tr><td><code>set KEY even</code></td><td>Imposta slot pari</td></tr>
<tr><td><code>set KEY odd</code></td><td>Imposta slot dispari</td></tr>
<tr><td><code>set KEY 1500</code></td><td>Sposta il TX audio offset verso 1500 Hz</td></tr>
<tr><td><code>set KEY exit</code></td><td>Chiude WSJT-X e termina cqrun</td></tr>
</table>

<h3>Note</h3>

<p>
La porta <code>4343</code> può essere raggiungibile dalla LAN. La chiave
protegge i comandi, ma viaggia in chiaro: usare solo su rete fidata, firewall,
VPN o tunnel SSH.
</p>
