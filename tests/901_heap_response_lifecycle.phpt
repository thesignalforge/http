--TEST--
Response object lifecycle - memory leak prevention
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;
use Signalforge\NativeHttp\Stream;

// ARRANGE: Test Response object memory management

echo "=== Test 1: Response immutability chain (header operations) ===\n";
// Tests proper cleanup when creating many intermediate objects
$base = new Response();
$r1 = $base->withStatus(200);
$r2 = $r1->withHeader('Content-Type', 'text/plain');
$r3 = $r2->withAddedHeader('X-Custom', 'value1');
$r4 = $r3->withAddedHeader('X-Custom', 'value2');
$r5 = $r4->withoutHeader('X-Custom');

// ASSERT: Final object should be valid
var_dump($r5->getStatusCode() === 200);
var_dump($r5->hasHeader('Content-Type') === true);
var_dump($r5->hasHeader('X-Custom') === false);
echo "Header chain passed\n";

echo "\n=== Test 2: getHeaderLine() with empty headers (edge case from heap fix) ===\n";
// This specifically tests the smart_str_0() edge case fix
$response = new Response();
$response = $response->withHeader('Empty-Header', '');
$line = $response->getHeaderLine('Empty-Header');

// ASSERT: Should return empty string, not crash
var_dump($line === '');
echo "Empty header passed\n";

echo "\n=== Test 3: Multiple withStatus() calls ===\n";
// Tests that status_text zend_string is properly managed
$resp = new Response();
for ($i = 200; $i <= 210; $i++) {
    $resp = $resp->withStatus($i);
    // ASSERT: Status should be correct
    var_dump($resp->getStatusCode() === $i);
}
echo "Status chain passed\n";

echo "\n=== Test 4: Body replacement stress test ===\n";
// Tests zval body management in immutability
$resp = new Response();
for ($i = 0; $i < 20; $i++) {
    // Create new stream for each iteration
    $body = Stream::fromString("Content iteration {$i}\n");
    $resp = $resp->withBody($body);
}

// ASSERT: Body should be accessible
$final_body = (string)$resp->getBody();
var_dump(strlen($final_body) > 0);
echo "Body replacement passed\n";

echo "\n=== Test 5: Rapid Response creation/destruction ===\n";
for ($i = 0; $i < 100; $i++) {
    $r = new Response();
    $r = $r->withStatus(200 + ($i % 100));
    $r = $r->withHeader('X-Test', "value{$i}");
    $status = $r->getStatusCode();
    // Object destroyed at end of loop
}
echo "Rapid lifecycle passed\n";

echo "\n=== Test 6: Protocol version chains ===\n";
$resp = new Response();
$resp = $resp->withProtocolVersion('1.0');
$resp = $resp->withProtocolVersion('1.1');
$resp = $resp->withProtocolVersion('2.0');

// ASSERT: Protocol version should be correct
var_dump($resp->getProtocolVersion() === '2.0');
echo "Protocol chain passed\n";

echo "\nAll Response heap tests passed\n";
?>
--EXPECT--
=== Test 1: Response immutability chain (header operations) ===
bool(true)
bool(true)
bool(true)
Header chain passed

=== Test 2: getHeaderLine() with empty headers (edge case from heap fix) ===
bool(true)
Empty header passed

=== Test 3: Multiple withStatus() calls ===
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
Status chain passed

=== Test 4: Body replacement stress test ===
bool(true)
Body replacement passed

=== Test 5: Rapid Response creation/destruction ===
Rapid lifecycle passed

=== Test 6: Protocol version chains ===
bool(true)
Protocol chain passed

All Response heap tests passed
