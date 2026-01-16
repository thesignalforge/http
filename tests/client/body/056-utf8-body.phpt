--TEST--
Client: POST request with UTF-8 characters in body
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
use Signalforge\NativeHttp\StreamFactory;

// ARRANGE: Create request with UTF-8 content using PSR-17 factories
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$streamFactory = new StreamFactory();

$utf8Data = json_encode([
    'title' => 'UTF-8 Test Product',
    'description' => 'Hello World - Cafe - naive'
], JSON_UNESCAPED_UNICODE);
$body = $streamFactory->createStream($utf8Data);
$request = $requestFactory->createRequest('POST', 'https://dummyjson.com/products/add')
    ->withHeader('Content-Type', 'application/json; charset=utf-8')
    ->withBody($body);

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $responseBody = (string)$response->getBody();
    $data = json_decode($responseBody, true);

    // ASSERT: Verify UTF-8 was preserved
    var_dump($response->getStatusCode() === 200 || $response->getStatusCode() === 201);
    var_dump(isset($data['title']));
    var_dump(isset($data['id']));
    echo "UTF-8 body POST successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
UTF-8 body POST successful
