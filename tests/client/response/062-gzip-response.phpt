--TEST--
Client: Response with gzip encoding (auto-decoded)
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

// ARRANGE: Create request with Accept-Encoding for gzip using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');
$request = $request->withHeader('Accept-Encoding', 'gzip, deflate');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $body = (string)$response->getBody();
    $data = json_decode($body, true);

    // ASSERT: Verify response was decoded (curl handles this automatically)
    var_dump($response->getStatusCode() === 200);
    var_dump(is_array($data));
    var_dump(isset($data['id']));
    echo "Gzip response decoded successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
Gzip response decoded successfully
