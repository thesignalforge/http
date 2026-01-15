--TEST--
Client constructs with retry configuration
--EXTENSIONS--
signalforge_http
--SKIPIF--
<?php
if (!class_exists('Signalforge\NativeHttp\Client')) {
    die('skip PSR-18 client not available (requires libcurl)');
}
?>
--FILE--
<?php
use Signalforge\NativeHttp\Client;

// Arrange: Configure retry with exponential backoff
$options = [
    'retry' => [
        'max' => 3,
        'delay' => 500,
        'max_delay' => 5000,
        'backoff' => 2.0,
    ],
];

// Act: Create client with retry config
$client = new Client($options);

// Assert: Client should be created successfully
var_dump($client instanceof Client);
?>
--EXPECT--
bool(true)
