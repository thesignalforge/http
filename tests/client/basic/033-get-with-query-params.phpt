--TEST--
Client: GET request with query parameters
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

// ARRANGE: Create request with query parameters using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$url = 'https://dummyjson.com/products/search?q=phone&limit=5';
$request = $requestFactory->createRequest('GET', $url);

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $body = (string)$response->getBody();
    $data = json_decode($body, true);

    // ASSERT: Verify query parameters were processed
    var_dump($response->getStatusCode() === 200);
    var_dump(isset($data['products']));
    var_dump(isset($data['limit']));
    var_dump($data['limit'] === 5);
    echo "Query parameters sent successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
Query parameters sent successfully
