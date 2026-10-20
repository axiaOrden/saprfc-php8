<?php

require_once dirname(__DIR__, 2) . '/config/bootstrap.php';

function envValue($key, $default = '')
{
    $value = getenv($key);
    return $value === false ? $default : $value;
}

function escapeHtml($value)
{
    return htmlspecialchars((string) $value, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}

$districts = [];
$error = null;
$rfc = null;
$connection = null;
$debug = filter_var(envValue('APP_DEBUG', 'false'), FILTER_VALIDATE_BOOLEAN);

try {
    if (!extension_loaded('saprfc')) {
        throw new RuntimeException('SAPRFC extension is not loaded.');
    }

    if (!envValue('SAP_USER') || !envValue('SAP_PASS')) {
        throw new RuntimeException('SAP credentials are missing in .sap.env.');
    }

    $settings = [
        'ASHOST' => envValue('SAP_ASHOST'),
        'SYSNR'  => envValue('SAP_SYSNR', '00'),
        'CLIENT' => envValue('SAP_CLIENT', '100'),
        'USER'   => envValue('SAP_USER'),
        'PASSWD' => envValue('SAP_PASS'),
        'LANG'   => envValue('SAP_LANG', 'EN'),
        'TRACE'  => envValue('SAP_TRACE', '0'),
    ];

    if (envValue('SAP_GWHOST') !== '') {
        $settings['GWHOST'] = envValue('SAP_GWHOST');
    }

    if (envValue('SAP_GWSERV') !== '') {
        $settings['GWSERV'] = envValue('SAP_GWSERV');
    }

    $connection = saprfc_open($settings);

    if (!$connection) {
        throw new RuntimeException('SAP connection failed: ' . saprfc_error());
    }

    $rfc = saprfc_function_discover($connection, 'RFC_READ_TABLE');

    if (!$rfc) {
        throw new RuntimeException('Unable to discover RFC_READ_TABLE.');
    }

    saprfc_import($rfc, 'QUERY_TABLE', 'T171T');
    saprfc_import($rfc, 'DELIMITER', '|');
    saprfc_import($rfc, 'ROWCOUNT', 1000);

    saprfc_table_init($rfc, 'FIELDS');
    saprfc_table_append($rfc, 'FIELDS', ['FIELDNAME' => 'BZIRK']);
    saprfc_table_append($rfc, 'FIELDS', ['FIELDNAME' => 'BZTXT']);

    saprfc_table_init($rfc, 'OPTIONS');
    saprfc_table_append($rfc, 'OPTIONS', [
        'TEXT' => "SPRAS = 'E'"
    ]);

    saprfc_table_init($rfc, 'DATA');

    $result = saprfc_call_and_receive($rfc);

    if ($result !== SAPRFC_OK) {
        throw new RuntimeException('RFC_READ_TABLE failed.');
    }

    $count = saprfc_table_rows($rfc, 'DATA');

    for ($i = 1; $i <= $count; $i++) {
        $row = saprfc_table_read($rfc, 'DATA', $i);
        $values = explode('|', $row['WA'] ?? '', 2);

        $districts[] = [
            'code' => trim($values[0] ?? ''),
            'description' => trim($values[1] ?? ''),
        ];
    }

} catch (Throwable $e) {
    error_log('Sales District Viewer: ' . str_replace(["\r", "\n"], ' ', $e->getMessage()));
    $error = $debug
        ? $e->getMessage()
        : 'Unable to retrieve sales districts. Check the server log for details.';
} finally {
    if ($rfc) {
        saprfc_function_free($rfc);
    }

    if ($connection) {
        saprfc_close($connection);
    }
}

?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>SAP Sales Districts</title>
    <style>
        body {
            font-family: system-ui, sans-serif;
            background: #f5f6f8;
            color: #20242a;
            margin: 0;
            padding: 40px 20px;
        }
        main {
            max-width: 850px;
            margin: auto;
        }
        h1 { margin-bottom: 8px; }
        p { color: #667085; }
        table {
            width: 100%;
            border-collapse: collapse;
            background: white;
            border-radius: 12px;
            overflow: hidden;
        }
        th, td {
            padding: 14px 18px;
            text-align: left;
            border-bottom: 1px solid #eee;
        }
        th { background: #e8edf5; }
        .error {
            background: #fee;
            padding: 16px;
            border-radius: 8px;
            color: #a22;
        }
    </style>
</head>
<body>
<main>
    <h1>SAP Sales Districts</h1>
    <p>Live SAP ECC data via RFC_READ_TABLE — T171T</p>

    <?php if ($error): ?>
        <div class="error"><?= escapeHtml($error) ?></div>
    <?php else: ?>
        <p><?= count($districts) ?> districts retrieved</p>
        <table>
            <thead>
                <tr>
                    <th>Sales District</th>
                    <th>Description</th>
                </tr>
            </thead>
            <tbody>
                <?php foreach ($districts as $district): ?>
                    <tr>
                        <td><?= escapeHtml($district['code']) ?></td>
                        <td><?= escapeHtml($district['description']) ?></td>
                    </tr>
                <?php endforeach; ?>
            </tbody>
        </table>
    <?php endif; ?>
</main>
</body>
</html>
