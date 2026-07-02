<html>
<pre>
cd /home/gmazzini/gm2
sh cqlinux/go
php cqlinux/ts890s_on.php
ping 10.0.0.10
</pre>

<h2>cqrun WHOIS interface</h2>

<p>
cqrun provides a small text-based TCP interface on port <code>4343</code>.
It can be used to read status, received decodes and diagnostics from another host.
All commands require <code>KEY</code>.
</p>

<h3>Usage</h3>

<pre><code>printf "KEY status\r\n" | nc 10.0.0.8 4343
printf "KEY version\r\n" | nc 10.0.0.8 4343
printf "KEY cqed\r\n" | nc 10.0.0.8 4343
printf "KEY rxed\r\n" | nc 10.0.0.8 4343
printf "KEY freefreq\r\n" | nc 10.0.0.8 4343</code></pre>

<p>
Control commands use the historical <code>set</code> form:
</p>

<pre><code>printf "set KEY ft8\r\n" | nc 10.0.0.8 4343
printf "set KEY ft4\r\n" | nc 10.0.0.8 4343
printf "set KEY even\r\n" | nc 10.0.0.8 4343
printf "set KEY odd\r\n" | nc 10.0.0.8 4343
printf "set KEY 1500\r\n" | nc 10.0.0.8 4343</code></pre>

<h3>Read commands</h3>

<table>
<tr><th>Command</th><th>Description</th></tr>
<tr><td><code>KEY help</code></td><td>Show available commands</td></tr>
<tr><td><code>KEY version</code></td><td>cqrun release, build date, WSJT-X version and uptime</td></tr>
<tr><td><code>KEY heartbeat</code></td><td>Age of the last WSJT-X heartbeat</td></tr>
<tr><td><code>KEY status</code></td><td>Frequency, mode, Enable TX, RX DF and TX DF</td></tr>
<tr><td><code>KEY rxed</code></td><td>Decodes received from WSJT-X</td></tr>
<tr><td><code>KEY cqed</code></td><td>CQ selection diagnostics</td></tr>
<tr><td><code>KEY freefreq</code></td><td>Free audio ranges in the 200-3000 Hz window</td></tr>
<tr><td><code>KEY used</code></td><td>Calls already selected/used</td></tr>
<tr><td><code>KEY logged</code></td><td>QSOs already present in the ADIF log</td></tr>
<tr><td><code>KEY escluded</code></td><td>Blacklisted calls; alias: <code>excluded</code></td></tr>
<tr><td><code>KEY read N</code></td><td>Read an internal numeric value</td></tr>
</table>

<h3>Control commands</h3>

<table>
<tr><th>Command</th><th>Action</th></tr>
<tr><td><code>set KEY ft8</code></td><td>Switch WSJT-X to FT8</td></tr>
<tr><td><code>set KEY ft4</code></td><td>Switch WSJT-X to FT4</td></tr>
<tr><td><code>set KEY even</code></td><td>Select even TX period</td></tr>
<tr><td><code>set KEY odd</code></td><td>Select odd TX period</td></tr>
<tr><td><code>set KEY 1500</code></td><td>Move TX audio offset toward 1500 Hz</td></tr>
<tr><td><code>set KEY exit</code></td><td>Close WSJT-X and terminate cqrun</td></tr>
</table>

<h3>Security note</h3>

<p>
Port <code>4343</code> may be reachable from the LAN. The key protects all
commands, but it is sent in clear text. Use this interface only on a trusted
network, or protect it with a firewall, VPN or SSH tunnel.
</p>
