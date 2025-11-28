import fastgpx


class TestBounds:

    # fastgpx.Bounds.__init__

    def test_init_defaults(self):
        bounds = fastgpx.Bounds()
        assert bounds.is_empty()
        assert bounds.min is None
        assert bounds.min is None
        assert bounds.max is None
        assert bounds.max is None

    def test_init_with_latlongs_positionals(self):
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

    def test_init_with_latlongs_keywords(self):
        ll1 = fastgpx.LatLong(-10, -5)
        ll2 = fastgpx.LatLong(30, 25)
        bounds = fastgpx.Bounds(min=ll1, max=ll2)
        assert not bounds.is_empty()
        assert bounds.min is not None
        assert bounds.max is not None

        assert bounds.min.latitude == -10.0
        assert bounds.min.longitude == -5.0
        assert bounds.max.latitude == 30.0
        assert bounds.max.longitude == 25.0

    def test_init_with_two_tuples_positionals(self):
        bounds = fastgpx.Bounds((-10, -5), (30, 25))
        assert not bounds.is_empty()
        assert bounds.min is not None
        assert bounds.min.latitude == -10.0
        assert bounds.min.longitude == -5.0
        assert bounds.max is not None
        assert bounds.max.latitude == 30.0
        assert bounds.max.longitude == 25.0

    def test_init_with_two_tuples_keywords(self):
        bounds = fastgpx.Bounds(min=(-10, -5), max=(30, 25))
        assert not bounds.is_empty()
        assert bounds.min is not None
        assert bounds.min.latitude == -10.0
        assert bounds.min.longitude == -5.0
        assert bounds.max is not None
        assert bounds.max.latitude == 30.0
        assert bounds.max.longitude == 25.0

    def test_init_with_three_tuples_positionals(self):
        bounds = fastgpx.Bounds((-10, -5, 68), (30, 25, 150))
        assert not bounds.is_empty()
        assert bounds.min is not None
        assert bounds.min.latitude == -10.0
        assert bounds.min.longitude == -5.0
        assert bounds.min.elevation == 68.0
        assert bounds.max is not None
        assert bounds.max.latitude == 30.0
        assert bounds.max.longitude == 25.0
        assert bounds.max.elevation == 150.0

    def test_init_with_three_tuples_keywords(self):
        bounds = fastgpx.Bounds(min=(-10, -5, 68), max=(30, 25, 150))
        assert not bounds.is_empty()
        assert bounds.min is not None
        assert bounds.min.latitude == -10.0
        assert bounds.min.longitude == -5.0
        assert bounds.min.elevation == 68.0
        assert bounds.max is not None
        assert bounds.max.latitude == 30.0
        assert bounds.max.longitude == 25.0
        assert bounds.max.elevation == 150.0

    # fastgpx.Bounds.__eq__

    def test_equality(self):
        bounds1 = fastgpx.Bounds((-10, -5, 68), (30, 25, 150))
        bounds2 = fastgpx.Bounds(min=(-10, -5, 68), max=(30, 25, 150))
        bounds3 = fastgpx.Bounds((-10, -5), (30, 25))

        assert bounds1 == bounds2
        assert bounds1 != bounds3

    # fastgpx.Bounds.add

    def test_add_latlong(self):
        bounds = fastgpx.Bounds()
        assert bounds.is_empty()

        ll1 = fastgpx.LatLong(-10, -5)
        bounds.add(ll1)
        assert not bounds.is_empty()
        assert bounds.min is not None
        assert bounds.min == ll1
        assert bounds.max is not None
        assert bounds.max == ll1

        ll2 = fastgpx.LatLong(30, 25)
        bounds.add(ll2)
        assert not bounds.is_empty()
        assert bounds.min is not None
        assert bounds.min.latitude == -10.0
        assert bounds.min.longitude == -5.0
        assert bounds.max is not None
        assert bounds.max.latitude == 30.0
        assert bounds.max.longitude == 25.0

    # fastgpx.Bounds.max_bounds

    def test_max_bounds(self):
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

    # fastgpx.Bounds.__repr__

    def test_repr(self):
        ll1 = fastgpx.LatLong(-10.3, -5.2, 68.5)
        ll2 = fastgpx.LatLong(30.3, 25.7, 150.2)
        bounds = fastgpx.Bounds(ll1, ll2)
        repr_str = repr(bounds)
        assert repr_str == "fastgpx.Bounds(min=(-10.3, -5.2, 68.5), max=(30.3, 25.7, 150.2))"

    # fastgpx.Bounds.__str__

    def test_str(self):
        ll1 = fastgpx.LatLong(-10.3, -5.2, 68.5)
        ll2 = fastgpx.LatLong(30.3, 25.7, 150.2)
        bounds = fastgpx.Bounds(ll1, ll2)
        str_str = str(bounds)
        assert str_str == "Bounds(min=(-10.3, -5.2, 68.5), max=(30.3, 25.7, 150.2))"
