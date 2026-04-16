<?php
/**
 * Signalforge HTTP Extension
 * Uri.stub.php - IDE stub for the Uri class
 *
 * @package Signalforge\Http
 */

declare(strict_types=1);

namespace Signalforge\NativeHttp;

/**
 * RFC 3986 URI value object implementing PSR-7 UriInterface.
 *
 * The implementation is immutable: every `with*()` method returns a new
 * instance with the requested change applied. Empty getters return empty
 * strings; {@see getPort()} returns null both when unset and when the
 * port matches the scheme's default (80 for http, 443 for https, etc.).
 *
 * @example
 * $uri = Uri::fromString('https://user:pass@example.com:8443/path?q=1#top');
 * $clean = $uri->withUserInfo('')->withFragment('');
 * echo (string) $clean; // https://example.com:8443/path?q=1
 */
final class Uri implements \Psr\Http\Message\UriInterface
{
    /**
     * Parse a URI string into a Uri instance.
     *
     * @param string $uri RFC 3986 URI
     * @return static
     */
    public static function fromString(string $uri): static {}

    /**
     * Scheme component (lowercased), or empty string if absent.
     *
     * @return string
     */
    public function getScheme(): string {}

    /**
     * "user[:password]" form, or empty string if no userinfo is set.
     *
     * @return string
     */
    public function getUserInfo(): string {}

    /**
     * Host component (lowercased), or empty string if absent.
     *
     * @return string
     */
    public function getHost(): string {}

    /**
     * Port component, or null if unset OR equal to the scheme's default.
     *
     * @return int|null
     */
    public function getPort(): ?int {}

    /**
     * Path component (may be empty).
     *
     * @return string
     */
    public function getPath(): string {}

    /**
     * Query string without the leading "?", or empty string if absent.
     *
     * @return string
     */
    public function getQuery(): string {}

    /**
     * Fragment without the leading "#", or empty string if absent.
     *
     * @return string
     */
    public function getFragment(): string {}

    /**
     * Authority component "[user-info@]host[:port]", or empty string if no host.
     *
     * @return string
     */
    public function getAuthority(): string {}

    /**
     * Recompose all components per RFC 3986 §5.3.
     *
     * @return string
     */
    public function __toString(): string {}

    /**
     * Return a new instance with the scheme replaced (lowercased).
     *
     * @param string $scheme
     * @return static
     */
    public function withScheme(string $scheme): static {}

    /**
     * Return a new instance with the user info replaced.
     *
     * @param string $user Empty string clears the user info entirely
     * @param string|null $password Optional password; ignored when user is empty
     * @return static
     */
    public function withUserInfo(string $user, ?string $password = null): static {}

    /**
     * Return a new instance with the host replaced (lowercased).
     *
     * @param string $host
     * @return static
     */
    public function withHost(string $host): static {}

    /**
     * Return a new instance with the port replaced.
     *
     * @param int|null $port Null clears the port; values must be in 0..65535
     * @return static
     * @throws \InvalidArgumentException If port is out of range
     */
    public function withPort(?int $port): static {}

    /**
     * Return a new instance with the path replaced.
     *
     * @param string $path Must not contain "?" or "#"
     * @return static
     * @throws \InvalidArgumentException If path contains a query or fragment delimiter
     */
    public function withPath(string $path): static {}

    /**
     * Return a new instance with the query string replaced.
     *
     * @param string $query Leading "?" is stripped if present; must not contain "#"
     * @return static
     * @throws \InvalidArgumentException If query contains a fragment delimiter
     */
    public function withQuery(string $query): static {}

    /**
     * Return a new instance with the fragment replaced.
     *
     * @param string $fragment Leading "#" is stripped if present
     * @return static
     */
    public function withFragment(string $fragment): static {}
}
