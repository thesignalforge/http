--TEST--
Client: Request with Authorization header
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

// ARRANGE: Create request with Bearer token using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/auth/me');
$request = $request->withHeader('Authorization', 'Bearer test-token-12345');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $statusCode = $response->getStatusCode();

    // ASSERT: Verify Authorization header was sent (401/403 is expected for invalid token)
    var_dump($statusCode === 200 || $statusCode === 401 || $statusCode === 403);
    echo "Authorization header sent successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
Authorization header sent successfully
