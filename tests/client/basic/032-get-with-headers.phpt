--TEST--
Client: GET request with custom headers
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

// ARRANGE: Create request with custom headers using PSR-17 factory
$client = new Client(['pool_size' => 2, 'timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');
$request = $request->withHeader('X-Custom-Header', 'test-value');
$request = $request->withHeader('Accept', 'application/json');
$request = $request->withHeader('User-Agent', 'Signalforge\NativeHttp/1.0');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $body = (string)$response->getBody();
    $data = json_decode($body, true);

    // ASSERT: Verify request succeeded and returned product data
    var_dump($response->getStatusCode() === 200);
    var_dump(isset($data['id']));
    var_dump($response->hasHeader('Content-Type'));
    echo "Headers sent successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
Headers sent successfully
