import fastgpx


class TestLatLong:

    # fastgpx.LatLong.__init__

    def test_init_with_two_floats_positionals(self):
        latlong = fastgpx.LatLong(12.34, 56.78)
        assert latlong.latitude == 12.34
        assert latlong.longitude == 56.78
        assert latlong.elevation == 0.0

    def test_init_with_three_floats_positionals(self):
        latlong = fastgpx.LatLong(12.345678, 56.789012, 89.2)
        assert latlong.latitude == 12.345678
        assert latlong.longitude == 56.789012
        assert latlong.elevation == 89.2

    def test_init_with_two_integers_positionals(self):
        latlong = fastgpx.LatLong(12, 56)
        assert latlong.latitude == 12.0
        assert latlong.longitude == 56.0
        assert latlong.elevation == 0.0

    def test_init_with_three_integers_positionals(self):
        latlong = fastgpx.LatLong(12, 56, 89)
        assert latlong.latitude == 12.0
        assert latlong.longitude == 56.0
        assert latlong.elevation == 89.0

    def test_init_with_two_floats_keywords(self):
        latlong = fastgpx.LatLong(latitude=12.34, longitude=56.78)
        assert latlong.latitude == 12.34
        assert latlong.longitude == 56.78
        assert latlong.elevation == 0.0

    def test_init_with_three_floats_keywords(self):
        latlong = fastgpx.LatLong(latitude=12.345678, longitude=56.789012, elevation=89.2)
        assert latlong.latitude == 12.345678
        assert latlong.longitude == 56.789012
        assert latlong.elevation == 89.2

    def test_init_with_two_integers_keywords(self):
        latlong = fastgpx.LatLong(latitude=12, longitude=56)
        assert latlong.latitude == 12.0
        assert latlong.longitude == 56.0
        assert latlong.elevation == 0.0

    def test_init_with_three_integers_keywords(self):
        latlong = fastgpx.LatLong(latitude=12, longitude=56, elevation=89)
        assert latlong.latitude == 12.0
        assert latlong.longitude == 56.0
        assert latlong.elevation == 89.0

    # fastgpx.LatLong.__eq__

    def test_equality(self):
        latlong1 = fastgpx.LatLong(12.34, 56.78, 90.1)
        latlong2 = fastgpx.LatLong(12.34, 56.78, 90.1)
        latlong3 = fastgpx.LatLong(12.34, 56.78, 91.2)

        assert latlong1 == latlong2
        assert latlong1 != latlong3

    # fastgpx.LatLong.__repr__

    def test_repr(self):
        latlong = fastgpx.LatLong(12.34, 56.78, 89.2)
        repr_str = repr(latlong)
        assert repr_str == "fastgpx.LatLong(latitude=12.34, longitude=56.78, elevation=89.2)"

    # fastgpx.LatLong.__str__

    def test_str(self):
        latlong = fastgpx.LatLong(12.34, 56.78, 89.2)
        str_str = str(latlong)
        assert str_str == "LatLong(12.34, 56.78, 89.2)"
