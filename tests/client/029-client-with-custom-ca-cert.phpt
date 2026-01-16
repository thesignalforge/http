--TEST--
Client configures with custom CA certificate
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

// Arrange: Configure custom CA cert path
$options = ['ca_cert' => '/etc/ssl/certs/ca-certificates.crt'];

// Act: Create client with custom CA cert
$client = new Client($options);

// Assert: Client should be created
var_dump($client instanceof Client);
echo "Client created with custom CA cert\n";
?>
--EXPECT--
bool(true)
Client created with custom CA cert
