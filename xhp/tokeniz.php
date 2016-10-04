#!/usr/bin/php7.0
<?php

$parser_y = 'parser.y';

$fo = fopen($parser_y . '.new', 'w');

$fh = fopen($parser_y, 'r')
    or exit("Error: unable to open file ($parser_y)\n");

$watch_token = true;

$found_tokens = array();
while (($buf = fgets($fh, 4096)) !== false) {
    if ($watch_token) {
        if (preg_match('/^%(token|left|right|nonassoc)\s+(.*)\s*$/', $buf, $matches)) {
            $rule = $matches[1];
            $tokens = explode(' ', trim($matches[2]));
            if ($rule == 'token') {
                $tok = $tokens[0];
                if (defined($tok)) {
                    $buf = "%token " . $tokens[0]." ".constant($tok). PHP_EOL;
                }
                if (array_key_exists($tok, $found_tokens)) {
                    unset($found_tokens[$tok]);
                }
            } else {
                foreach ($tokens as $tok) {
                    if ($tok[0] != "'") $found_tokens[$tok] = true;
                }
            }
        }
        if (preg_match('/^%%\s*$/', $buf)) {

            foreach(array_keys($found_tokens) as $tok) {
                if (defined($tok)) {
                    $buf = "%token " . $tok." ".constant($tok). PHP_EOL . $buf;
                }
            }

            $watch_token = false;
        }
    }
    fputs($fo, $buf);
}

if (!feof($fh)) {
    exit("Error: unexpected fgets() fail\n");
}

fclose($fh);

fclose($fo);

