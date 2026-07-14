#!/usr/bin/php8.4
<?php
header("Content-Type: text/plain");
echo "\n";
foreach ($_SERVER as $key => $value) {
    echo "$key=$value\n";
}
?>
