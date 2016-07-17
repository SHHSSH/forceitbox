<?php
date_default_timezone_set('UTC');
$args = array_slice($argv, 1);
printf("%d\n", strtotime(implode(' ', $args)));
