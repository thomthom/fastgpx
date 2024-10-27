import fastgpx

# fastgpx.Bounds.__init__


def test_bounds_init_defaults():
    bounds = fastgpx.Bounds()
    assert bounds.is_empty()
    assert bounds.min is None
    assert bounds.min is None
    assert bounds.max is None
    assert bounds.max is None


def test_bounds_init_with_latlongs():
    ll1 = fastgpx.LatLong(-10, -5)
    ll2 = fastgpx.LatLong(30, 25)
    bounds = fastgpx.Bounds(ll1, ll2)
    assert not bounds.is_empty()
    assert bounds.min is not None
    assert bounds.max is not None

    assert bounds.min.latitude == -10.0
    assert bounds.min.longitude == -5.0
    assert bounds.max.latitude == 30.0
    assert bounds.max.longitude == 25.0

    assert bounds.min.latitude == bounds.min_latitude
    assert bounds.min.longitude == bounds.min_longitude
    assert bounds.max.latitude == bounds.max_latitude
    assert bounds.max.longitude == bounds.max_longitude


def test_bounds_init_with_tuples():
    bounds = fastgpx.Bounds((-10, -5), (30, 25))
    assert not bounds.is_empty()
    assert bounds.min is not None
    assert bounds.min.latitude == -10.0
    assert bounds.min.longitude == -5.0
    assert bounds.max is not None
    assert bounds.max.latitude == 30.0
    assert bounds.max.longitude == 25.0

# fastgpx.Bounds.test_bounds_max_bounds


def test_bounds_max_bounds():
    bounds1 = fastgpx.Bounds((-10, -5), (30, 25))
    bounds2 = fastgpx.Bounds((5, 30), (40, 20))
    bounds = bounds1.max_bounds(bounds2)
    assert not bounds.is_empty()
    assert bounds.min is not None
    assert bounds.min.latitude == -10.0
    assert bounds.min.longitude == -5.0
    assert bounds.max is not None
    assert bounds.max.latitude == 40.0
    assert bounds.max.longitude == 30.0
