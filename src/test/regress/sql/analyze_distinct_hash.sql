--
-- Exercise compute_distinct_stats() when hashable types allow hashed lookups.
--

SET client_min_messages TO WARNING;

--
-- Case 1: all values are distinct.  This forces the track[] array to fill
-- and then exercise the "drop tail item" path repeatedly.
--
DROP TABLE IF EXISTS analyze_distinct_hash_unique;
CREATE TABLE analyze_distinct_hash_unique (x xid);
ALTER TABLE analyze_distinct_hash_unique ALTER COLUMN x SET STATISTICS 100;
INSERT INTO analyze_distinct_hash_unique
SELECT i::text::xid FROM generate_series(1, 300) i;
ANALYZE analyze_distinct_hash_unique;

WITH m AS MATERIALIZED (
	SELECT string_to_array(trim(both '{}' from most_common_vals::text), ',') AS mcv
	FROM pg_stats
	WHERE schemaname = 'public'
	  AND tablename = 'analyze_distinct_hash_unique'
	  AND attname = 'x'
)
SELECT array_length(mcv, 1) AS mcv_len,
	   mcv[1] AS mcv_first,
	   mcv[100] AS mcv_100th
FROM m;

--
-- Case 2: bubble-up during repeated matches, exercising swaps while keeping
-- hashed indexes in sync.
--
DROP TABLE IF EXISTS analyze_distinct_hash_bubble;
CREATE TABLE analyze_distinct_hash_bubble (x xid);
ALTER TABLE analyze_distinct_hash_bubble ALTER COLUMN x SET STATISTICS 100;
INSERT INTO analyze_distinct_hash_bubble
SELECT i::text::xid FROM generate_series(1, 10) i;
INSERT INTO analyze_distinct_hash_bubble
SELECT '1'::xid FROM generate_series(1, 20);
ANALYZE analyze_distinct_hash_bubble;

SELECT most_common_vals::text
FROM pg_stats
WHERE schemaname = 'public'
  AND tablename = 'analyze_distinct_hash_bubble'
  AND attname = 'x';

DROP TABLE analyze_distinct_hash_unique;
DROP TABLE analyze_distinct_hash_bubble;

RESET client_min_messages;
