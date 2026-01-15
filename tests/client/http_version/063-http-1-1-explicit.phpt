--TEST--
Client: Request with HTTP/1.1 version explicitly set
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

// ARRANGE: Create client with HTTP/1.1 using PSR-17 factory
$client = new Client(['http_version' => '1.1', 'timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $protocolVersion = $response->getProtocolVersion();

    // ASSERT: Verify HTTP/1.1 was used
    var_dump($response->getStatusCode() === 200);
    var_dump($protocolVersion === '1.1');
    echo "HTTP/1.1 request successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
HTTP/1.1 request successful
