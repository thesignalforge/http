--TEST--
Uri: Security - Host header injection prevention
--EXTENSIONS--
signalforge_http
--FILE--
<?php
/**
 * Tests that the URI class properly rejects hosts containing injection characters
 * that could be used for HTTP response splitting or header injection attacks.
 *
 * Security: CVE-class prevention for HTTP header injection
 */

use Signalforge\NativeHttp\Uri;

echo "=== Test 1: Newline in host (CRLF injection) ===\n";
try {
    $uri = Uri::fromString("http://evil.com\r\nX-Injected: header/path");
    echo "FAIL: Should have thrown exception\n";
} catch (InvalidArgumentException $e) {
    echo "OK: Rejected host with CRLF\n";
}

echo "\n=== Test 2: Newline via withHost() ===\n";
try {
    $uri = Uri::fromString("http://example.com");
    $uri = $uri->withHost("evil.com\nX-Injected: header");
    echo "FAIL: Should have thrown exception\n";
} catch (InvalidArgumentException $e) {
    echo "OK: withHost() rejected injection chars\n";
}

echo "\n=== Test 3: Null byte in host ===\n";
try {
    $uri = Uri::fromString("http://evil.com\x00.example.com/path");
    echo "FAIL: Should have thrown exception\n";
} catch (InvalidArgumentException $e) {
    echo "OK: Rejected host with null byte\n";
}

echo "\n=== Test 4: Control character in host ===\n";
try {
    $uri = Uri::fromString("http://evil\x1fcom/path");
    echo "FAIL: Should have thrown exception\n";
} catch (InvalidArgumentException $e) {
    echo "OK: Rejected host with control character\n";
}

echo "\n=== Test 5: Tab character in host ===\n";
try {
    $uri = Uri::fromString("http://evil.com\t/path");
    echo "FAIL: Should have thrown exception\n";
} catch (InvalidArgumentException $e) {
    echo "OK: Rejected host with tab\n";
}

echo "\n=== Test 6: Carriage return only ===\n";
try {
    $uri = Uri::fromString("http://evil.com\rX-Injected: header/");
    echo "FAIL: Should have thrown exception\n";
} catch (InvalidArgumentException $e) {
    echo "OK: Rejected host with CR\n";
}

echo "\n=== Test 7: Valid host should work ===\n";
$uri = Uri::fromString("http://example.com/path");
echo "Host: " . $uri->getHost() . "\n";

echo "\n=== Test 8: IPv6 host should work ===\n";
$uri = Uri::fromString("http://[::1]:8080/path");
echo "Host: " . $uri->getHost() . "\n";

echo "\n=== Test 9: Subdomain with hyphen should work ===\n";
$uri = Uri::fromString("http://my-app.example.com/path");
echo "Host: " . $uri->getHost() . "\n";

echo "\n=== Test 10: Userinfo with injection chars ===\n";
try {
    $uri = Uri::fromString("http://user\r\nX-Injected:header@example.com/");
    echo "FAIL: Should have thrown exception\n";
} catch (InvalidArgumentException $e) {
    echo "OK: Rejected userinfo with injection chars\n";
}

echo "\n=== Test 11: withUserInfo() with injection chars ===\n";
try {
    $uri = Uri::fromString("http://example.com");
    $uri = $uri->withUserInfo("user\nX-Inject", "pass\nvalue");
    echo "FAIL: Should have thrown exception\n";
} catch (InvalidArgumentException $e) {
    echo "OK: withUserInfo() rejected injection chars\n";
}

echo "\n=== Test 12: Valid userinfo should work ===\n";
$uri = Uri::fromString("http://user:pass@example.com/path");
echo "UserInfo: " . $uri->getUserInfo() . "\n";

echo "\nAll security tests completed!\n";
?>
--EXPECT--
=== Test 1: Newline in host (CRLF injection) ===
OK: Rejected host with CRLF

=== Test 2: Newline via withHost() ===
OK: withHost() rejected injection chars

=== Test 3: Null byte in host ===
OK: Rejected host with null byte

=== Test 4: Control character in host ===
OK: Rejected host with control character

=== Test 5: Tab character in host ===
OK: Rejected host with tab

=== Test 6: Carriage return only ===
OK: Rejected host with CR

=== Test 7: Valid host should work ===
Host: example.com

=== Test 8: IPv6 host should work ===
Host: [::1]

=== Test 9: Subdomain with hyphen should work ===
Host: my-app.example.com

=== Test 10: Userinfo with injection chars ===
OK: Rejected userinfo with injection chars

=== Test 11: withUserInfo() with injection chars ===
OK: withUserInfo() rejected injection chars

=== Test 12: Valid userinfo should work ===
UserInfo: user:pass

All security tests completed!
