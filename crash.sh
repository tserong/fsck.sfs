#!/bin/bash

# So I want to run rgw (probably from a container, but initially just the
# binary from a local build, because it's quicker for me), dumping its
# output to some file, and I need to grab its PID after starting and
# backgrounding it so I can kill it later.
# Next, I want to spawn a bunch of s3cmd clients to "do things" to it:
# - create buckets
# - put objects (both single and multipart)
# - get objects
# - delete objects
# - delete buckets
# While that's happening, I want to randomly kill s3gw (and once it's
# dead, probably kill all the workers too), then run fsck.s3gw on the
# store and see what it finds.

if [ "$1" != "tserong" ] ; then
    echo "This script is hard coded to use Tim's dev paths."
    echo "Please don't run it without reading it first :-)"
    exit 1
fi

RUNDIR="$(mktemp -d --tmpdir s3gw-crash-XXXX)"
RADOSGW="/home/tserong/src/github/aquarist-labs/ceph/build/bin/radosgw"
FSCK="/home/tserong/src/github/tserong/fsck.s3gw/build/fsck.s3gw"
S3CFG="$RUNDIR/s3cmd.cfg"
LOGDIR="$RUNDIR/logs"
STOREDIR="$RUNDIR/store"

for prog in s3cmd pv ; do
    if ! $prog --version >/dev/null 2>&1 ; then
        echo "ERROR: $prog is not installed (please install it then try again)"
        exit 1
    fi
done

mkdir -p $LOGDIR $STORE
cat << EOF > $S3CFG
[default]
access_key = test
access_token =
add_encoding_exts =
add_headers =
bucket_location = US
ca_certs_file =
cache_file =
check_ssl_certificate = True
check_ssl_hostname = True
cloudfront_host = cloudfront.amazonaws.com
connection_max_age = 5
connection_pooling = True
content_disposition =
content_type =
default_mime_type = binary/octet-stream
delay_updates = False
delete_after = False
delete_after_fetch = False
delete_removed = False
dry_run = False
enable_multipart = True
encoding = UTF-8
encrypt = False
expiry_date =
expiry_days =
expiry_prefix =
follow_symlinks = False
force = False
get_continue = False
gpg_command = /usr/bin/gpg
gpg_decrypt = %(gpg_command)s -d --verbose --no-use-agent --batch --yes --passphrase-fd %(passphrase_fd)s -o %(output_file)s %(input_file)s
gpg_encrypt = %(gpg_command)s -c --verbose --no-use-agent --batch --yes --passphrase-fd %(passphrase_fd)s -o %(output_file)s %(input_file)s
gpg_passphrase = test
guess_mime_type = True
host_base = 127.0.0.1:7480
host_bucket = 127.0.0.1:7480/%(bucket)
human_readable_sizes = False
invalidate_default_index_on_cf = False
invalidate_default_index_root_on_cf = True
invalidate_on_cf = False
kms_key =
limit = -1
limitrate = 0
list_allow_unordered = False
list_md5 = False
log_target_prefix =
long_listing = False
max_delete = -1
mime_type =
multipart_chunk_size_mb = 15
multipart_copy_chunk_size_mb = 1024
multipart_max_chunks = 10000
preserve_attrs = True
progress_meter = True
proxy_host =
proxy_port = 0
public_url_use_https = False
put_continue = False
recursive = False
recv_chunk = 65536
reduced_redundancy = False
requester_pays = False
restore_days = 1
restore_priority = Standard
secret_key = test
send_chunk = 65536
server_side_encryption = False
signature_v2 = False
signurl_use_https = False
simpledb_host = sdb.amazonaws.com
skip_existing = False
socket_timeout = 300
ssl_client_cert_file =
ssl_client_key_file =
stats = False
stop_on_error = False
storage_class =
throttle_max = 100
upload_id =
urlencoding_mode = normal
use_http_expect = False
use_https = False
use_mime_magic = True
verbosity = WARNING
website_endpoint = http://%(bucket)s.s3-website-%(location)s.amazonaws.com/
website_error =
website_index = index.html
EOF

worker() {
    local run=0
    while true ; do
        local bucket="bucket$run"
        s3cmd -c $S3CFG mb s3://$bucket
        # This writes two versions of two objects
        for n in $(seq 1 2) ; do
            echo "Writing test"
            echo test | s3cmd -c $S3CFG put - s3://$bucket/test >/dev/null
            echo "Writing random (limited to 5M/sec)"
            dd if=/dev/urandom of=/dev/stdout bs=1M count=10 status=none | pv -q -L 5M | s3cmd -c $S3CFG put - s3://$bucket/random >/dev/null
        done
        s3cmd -c $S3CFG la
        s3cmd -c $S3CFG rm s3://$bucket/test
        s3cmd -c $S3CFG rm s3://$bucket/random
        s3cmd -c $S3CFG rb s3://$bucket
        run="$(( run + 1 ))"
    done
}

for run in $(seq 1 100) ; do
    echo "starting s3gw ($run)..."
    $RADOSGW -i s3gw -d --no-mon-config --debug-rgw 15 --rgw-backend-store sfs --rgw-data $STOREDIR --run-dir $RUNDIR --rgw-sfs-data-path $STOREDIR > $LOGDIR/s3gw.log &
    S3GW_PID=$!
    # give it an arbitrary 5 seconds to not fail to start
    for n in $(seq 1 5) ; do
        if ! kill -0 $S3GW_PID 2>/dev/null; then
            echo "s3gw terminated early for some reason - check $LOGDIR"
            exit 1
        fi
        sleep 1
    done
    echo "s3gw running with PID $S3GW_PID"

    worker &
    WORKER_PID=$!

    sleep $(( 10 + $RANDOM % 50 ))
    echo "killing s3gw"
    kill -9 $S3GW_PID

    echo "killing worker"
    kill -9 $WORKER_PID

    if ! $FSCK $STOREDIR ; then
        echo "fsck failed"
        echo "Logs are in $LOGDIR"
        echo "Store is in $STOREDIR"
        exit 1
    fi
done

rm -rf $RUNDIR
