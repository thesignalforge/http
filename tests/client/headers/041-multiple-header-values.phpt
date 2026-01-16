--TEST--
Client: Request with multiple values for same header
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

// ARRANGE: Create request with multiple Accept headers using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');
$request = $request->withAddedHeader('Accept', 'application/json');
$request = $request->withAddedHeader('Accept', 'text/html');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $body = (string)$response->getBody();
    $data = json_decode($body, true);

    // ASSERT: Verify request succeeded
    var_dump($response->getStatusCode() === 200);
    var_dump(isset($data['id']));
    echo "Multiple header values sent successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
Multiple header values sent successfully
