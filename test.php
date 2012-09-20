<?php

$data = array(
    'foo' => "abc",
    'bar' => 12345678,
    'baz' => 20,
    'wurst' => array( 'abc', 'def' ),
    'blar' => array( 'ghi', 'jklmn' ),
    'koffer' => array( 1, 3, 5, 7, 9, 11, 13 ),
    'blub' => array(
        array('id' => 1, 'name' => 'einseins'),
        array('id' => 2, 'name' => 'zweizwei'),
    ),
);

try {
    $packed = phproto_pack('TestRequest', $data);
} catch (Exception $e) {
    print "phproto_pack() failed: {$e->getMessage()}\n";  
    exit(1);
}

try {
    $unpacked = phproto_unpack('TestRequest', $packed);
} catch (Exception $e) {
    print "phproto_unpack() failed: {$e->getMessage()}\n";  
    exit(1);
}

$data = serialize($data);
$unpacked = serialize($unpacked);

if ($data === $unpacked) {
    print "data unchanged through pack/unpack.\n";
    return 0;
} else {
    print "data diff.\n";
    print $data ."\n". $unpacked ."\n";
    return 0;
}

