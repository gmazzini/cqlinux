<?php

$gmwin=shell_exec("xdotool search --onlyvisible --name 'K1JT'");
shell_exec("import -silent -window $gmwin x1.tif");
shell_exec("convert x1.tif -set colorspace Gray -separate -average x2.tif");
shell_exec("tesseract x2.tif x3 hocr");


?>
