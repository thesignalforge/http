--TEST--
Client: PSR-7 Request immutability verification
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

// ARRANGE: Create request and modify it using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request1 = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');
$request2 = $request1->withHeader('X-Test', 'value1');
$request3 = $request2->withHeader('X-Another', 'value2');

// ACT: Send different versions of the request
try {
    $response1 = $client->sendRequest($request1);
    $response2 = $client->sendRequest($request2);
    $response3 = $client->sendRequest($request3);

    // ASSERT: Verify all requests succeeded and were independent
    var_dump($response1->getStatusCode() === 200);
    var_dump($response2->getStatusCode() === 200);
    var_dump($response3->getStatusCode() === 200);
    var_dump($request1 !== $request2);
    var_dump($request2 !== $request3);
    echo "PSR-7 immutability verified\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
PSR-7 immutability verified
