--TEST--
Client: Request with HTTP/2 upgrade
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

// ARRANGE: Create client with HTTP/2 preference using PSR-17 factory
$client = new Client(['http_version' => '2', 'timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $protocolVersion = $response->getProtocolVersion();

    // ASSERT: Verify HTTP/2 was used (or fallback to 1.1)
    var_dump($response->getStatusCode() === 200);
    var_dump(in_array($protocolVersion, ['2', '2.0', '1.1']));
    echo "HTTP/2 request successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
HTTP/2 request successful
