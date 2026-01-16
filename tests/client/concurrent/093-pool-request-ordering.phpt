--TEST--
Client: Thread pool request queue ordering
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
/**
 * Tests that requests are processed and responses can be tracked
 * back to their original request context.
 */
use Signalforge\NativeHttp\Client;
use Signalforge\NativeHttp\HttpRequestPool;
use Signalforge\NativeHttp\RequestFactory;

$requestFactory = new RequestFactory();

echo "=== Test 1: Responses match request URLs ===\n";
$client = new Client(['pool_size' => 4, 'timeout' => 30]);
$pool = new HttpRequestPool($client, 10);

// Track which product IDs were requested
$requestedIds = [];
$receivedData = [];

for ($i = 1; $i <= 5; $i++) {
    $request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $i);
    $requestedIds[] = $i;
    $pool->add($request,
        function($response) use (&$receivedData, $i) {
            $body = (string)$response->getBody();
            $data = json_decode($body, true);
            if ($data && isset($data['id'])) {
                $receivedData[$i] = $data['id'];
            }
        },
        function($error) {}
    );
}

$responses = $pool->wait();

// All responses should have matching IDs
$allMatch = true;
foreach ($requestedIds as $expectedId) {
    if (!isset($receivedData[$expectedId]) || $receivedData[$expectedId] !== $expectedId) {
        $allMatch = false;
        break;
    }
}
var_dump($allMatch);
var_dump(count($receivedData) === 5);
echo "Response data matches request context\n";

echo "\n=== Test 2: Callback receives correct response ===\n";
$client2 = new Client(['pool_size' => 2, 'timeout' => 30]);
$pool2 = new HttpRequestPool($client2, 10);

$callbackResults = [];

// Request products with known IDs
$ids = [1, 10, 5];
foreach ($ids as $id) {
    $request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $id);
    $pool2->add($request,
        function($response) use (&$callbackResults, $id) {
            $body = (string)$response->getBody();
            $data = json_decode($body, true);
            $callbackResults[] = [
                'expected' => $id,
                'received' => $data['id'] ?? null,
                'status' => $response->getStatusCode()
            ];
        },
        function($error) use (&$callbackResults, $id) {
            $callbackResults[] = ['expected' => $id, 'error' => true];
        }
    );
}

$pool2->wait();

$correctCallbacks = 0;
foreach ($callbackResults as $result) {
    if (!isset($result['error']) && $result['expected'] === $result['received'] && $result['status'] === 200) {
        $correctCallbacks++;
    }
}
var_dump($correctCallbacks === 3);
echo "Callbacks received correct responses\n";

echo "\n=== Test 3: Responses array is complete ===\n";
$client3 = new Client(['pool_size' => 4, 'timeout' => 30]);
$pool3 = new HttpRequestPool($client3, 20);

for ($i = 1; $i <= 10; $i++) {
    $request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $i);
    $pool3->add($request);
}

$responses = $pool3->wait();

// Verify all responses are present
$validResponses = 0;
foreach ($responses as $response) {
    if ($response->getStatusCode() === 200) {
        $validResponses++;
    }
}
var_dump($validResponses === 10);
echo "All responses collected\n";

echo "\nAll request ordering tests passed!\n";
?>
--EXPECT--
=== Test 1: Responses match request URLs ===
bool(true)
bool(true)
Response data matches request context

=== Test 2: Callback receives correct response ===
bool(true)
Callbacks received correct responses

=== Test 3: Responses array is complete ===
bool(true)
All responses collected

All request ordering tests passed!
