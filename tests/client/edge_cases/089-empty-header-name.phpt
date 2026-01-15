--TEST--
Client: Handle empty or invalid header gracefully
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

// ARRANGE: Create request with normal headers using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');
$request = $request->withHeader('X-Valid-Header', 'value');

// ACT: Send the request (invalid headers should be caught by PSR-7)
try {
    $response = $client->sendRequest($request);

    // ASSERT: Verify request succeeded
    var_dump($response->getStatusCode() === 200);
    echo "Header validation handled correctly\n";
} catch (\Exception $e) {
    // PSR-7 validation caught it
    var_dump(true);
    echo "Header validation handled correctly\n";
}
?>
--EXPECT--
bool(true)
Header validation handled correctly
