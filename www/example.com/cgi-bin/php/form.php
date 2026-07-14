#!/usr/bin/php8.4
<?php
//   curl "http://host:port/php/form.php?name=alice"
//   curl -d "name=alice" http://host:port/php/form.php
header("Content-Type: text/html");
echo "<html><body>";
echo "<h1>Form Data</h1>";

$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
$params = [];

if ($method === 'POST') {
    $contentType = $_SERVER['CONTENT_TYPE'] ?? '';
    if (strpos($contentType, 'application/x-www-form-urlencoded') !== false) {
        parse_str(file_get_contents('php://input'), $params);
    } elseif (strpos($contentType, 'application/json') !== false) {
        $params = json_decode(file_get_contents('php://input'), true) ?? [];
    }
} else {
    parse_str($_SERVER['QUERY_STRING'] ?? '', $params);
}

echo "<p>Method: " . htmlspecialchars($method) . "</p>";
echo "<h2>Parameters:</h2><ul>";
foreach ($params as $key => $value) {
    if (is_array($value)) {
        foreach ($value as $val) {
            echo "<li><b>" . htmlspecialchars($key) . "</b>: " . htmlspecialchars($val) . "</li>";
        }
    } else {
        echo "<li><b>" . htmlspecialchars($key) . "</b>: " . htmlspecialchars($value) . "</li>";
    }
}
echo "</ul></body></html>";
?>
