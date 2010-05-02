#!/usr/bin/perl

use strict;
use warnings;

use Clutter::TestHelper tests => 5;

use Champlain;

exit tests();

sub tests {
	my $cache = Champlain::FileCache->new();
	isa_ok($cache, 'Champlain::FileCache');
	
	$cache = Champlain::FileCache->new_full(1_024 * 10, "cache", FALSE);
	isa_ok($cache, 'Champlain::FileCache');


	is($cache->get_size_limit, 1_024 * 10, "get_size_limit");
	$cache->set_size_limit(2_048);
	is($cache->get_size_limit, 2_048, "set_size_limit");
	
	is($cache->get_cache_dir, "cache", "get_cache_dir");
	
	# Can't be tested
	$cache->purge();
	$cache->purge_on_idle();

	return 0;
}
