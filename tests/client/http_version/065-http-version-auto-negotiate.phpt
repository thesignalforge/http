--TEST--
Client: Auto-negotiate HTTP version
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
use Signalforge\NativeHttp\RequestFactory;

// ARRANGE: Create client without specifying HTTP version (auto-negotiate) using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $protocolVersion = $response->getProtocolVersion();

    // ASSERT: Verify request succeeded with any HTTP version
    var_dump($response->getStatusCode() === 200);
    var_dump(in_array($protocolVersion, ['1.0', '1.1', '2', '2.0', '3']));
    echo "HTTP version auto-negotiation successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
HTTP version auto-negotiation successful
