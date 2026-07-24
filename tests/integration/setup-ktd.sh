#!/bin/sh
# Prepare a running koha-testing-docker instance for the KohaDownload
# integration test: enable the REST API, raise the report row limit,
# create the two saved reports, and report the values the test needs.
#
# Prints KEY=VALUE lines on stdout so a GitHub Actions step can append
# them straight to $GITHUB_ENV; everything else goes to stderr.
#
# CONTAINER defaults to the compose name used in CI; for a local ktd
# instance run with CONTAINER=kohadev-koha-1.

set -e

CONTAINER=${CONTAINER:-koha-koha-1}

koha_mysql() {
    docker exec -i "$CONTAINER" koha-mysql kohadev "$@"
}

koha_mysql >&2 <<'SQL'
UPDATE systempreferences SET value = '1' WHERE variable = 'RESTBasicAuth';
UPDATE systempreferences SET value = '1000000' WHERE variable = 'SvcMaxReportRows';
DELETE FROM saved_sql WHERE report_name IN ('KOC borrowers', 'KOC issues');
INSERT INTO saved_sql ( borrowernumber, date_created, last_modified, savedsql, report_name, type, notes, cache_expiry, public )
VALUES ( NULL, NOW(), NOW(),
 'SELECT b.borrowernumber, b.cardnumber, b.surname, b.firstname, b.address, b.city, b.phone, b.dateofbirth, COALESCE( ( SELECT SUM(a.amountoutstanding) FROM accountlines a WHERE a.borrowernumber = b.borrowernumber ), 0 ) AS total_fines FROM borrowers b',
 'KOC borrowers', '1', '', 300, 0 );
INSERT INTO saved_sql ( borrowernumber, date_created, last_modified, savedsql, report_name, type, notes, cache_expiry, public )
VALUES ( NULL, NOW(), NOW(),
 'SELECT i.borrowernumber, i.date_due, it.itemcallnumber, bib.title, COALESCE( it.itype, bi.itemtype ) AS itemtype FROM issues i JOIN items it ON it.itemnumber = i.itemnumber JOIN biblio bib ON bib.biblionumber = it.biblionumber JOIN biblioitems bi ON bi.biblionumber = bib.biblionumber',
 'KOC issues', '1', '', 300, 0 );
SQL

# Install the companion offline circulation plugin when its source is
# checked out somewhere, the plugin integration tests skip without it
if [ -n "$KOC_PLUGIN_DIR" ]; then
    docker cp "$KOC_PLUGIN_DIR/Koha" "$CONTAINER":/var/lib/koha/kohadev/plugins >&2
    docker exec "$CONTAINER" chown -R kohadev-koha:kohadev-koha /var/lib/koha/kohadev/plugins >&2
    docker exec "$CONTAINER" koha-shell kohadev -c 'perl /kohadevbox/koha/misc/devel/install_plugins.pl' >&2
    echo "KOHA_PLUGIN=1"
fi

# Sysprefs are cached, restart memcached and plack so the running
# server sees the changes made behind its back
docker restart "${CONTAINER%-koha-1}-memcached-1" >&2 || true
docker exec "$CONTAINER" koha-plack --restart kohadev >&2
sleep 5

BORROWERS_REPORT_ID=$(docker exec "$CONTAINER" koha-mysql kohadev -N -B -e \
    "SELECT id FROM saved_sql WHERE report_name = 'KOC borrowers'")
ISSUES_REPORT_ID=$(docker exec "$CONTAINER" koha-mysql kohadev -N -B -e \
    "SELECT id FROM saved_sql WHERE report_name = 'KOC issues'")
EXPECTED_PATRONS=$(docker exec "$CONTAINER" koha-mysql kohadev -N -B -e \
    "SELECT COUNT(*) FROM borrowers")

echo "KOHA_BORROWERS_REPORT_ID=$BORROWERS_REPORT_ID"
echo "KOHA_ISSUES_REPORT_ID=$ISSUES_REPORT_ID"
echo "KOHA_EXPECTED_PATRONS=$EXPECTED_PATRONS"
