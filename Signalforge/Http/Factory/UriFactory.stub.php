<?php
/**
 * Signalforge HTTP Extension
 * UriFactory.stub.php - IDE stub for UriFactory class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * PSR-17 compliant URI factory.
 *
 * Creates UriInterface instances from URI strings.
 *
 * @final
 * @implements \Psr\Http\Message\UriFactoryInterface
 */
final class UriFactory implements \Psr\Http\Message\UriFactoryInterface
{
    /**
     * Create a new URI.
     *
     * @param string $uri The URI to parse
     * @return \Psr\Http\Message\UriInterface
     * @throws \InvalidArgumentException If the given URI cannot be parsed
     */
    public function createUri(string $uri = ''): \Psr\Http\Message\UriInterface {}
}
