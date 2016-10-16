#!/usr/bin/env perl
use warnings;
use strict;
use utf8;
use 5.020;

use Test::Pdfout::Command tests => 10;
use Test::More;
use Testlib;

set_get_test(
    command => ['outline'],
    
    broken_input => [
    # no title
    '[{"page": 1}]',
    # page number too big
    '[{"title": "abc", "page": 11}]',
    # open not a bool
	'[{"title": "abc", "page": 1, "open": "a"}]'],
    
    input => <<'EOD'
[
  {
    "title": "title1",
    "page": 1,
    "view": [
      "XYZ",
      null,
      null,
      null
    ]
  },
  {
    "title": "title2",
    "page": 2,
    "view": [
      "XYZ",
      null,
      null,
      null
    ],
    "open": true,
    "kids": [
      {
        "title": "uiax ",
        "page": 3,
        "view": [
          "FitH",
          null
        ]
      },
      {
        "title": "mein schatzz",
        "page": 3,
        "view": [
          "FitBH",
          10
        ],
        "open": false,
        "kids": [
          {
            "title": "♦♥♠",
            "page": 3,
            "view": [
              "FitV",
              20
            ],
            "open": true,
            "kids": [
              {
                "title": "abc",
                "page": 4,
                "view": [
                  "XYZ",
                  null,
                  null,
                  2
                ]
              }
            ]
          },
          {
            "title": "abc",
            "page": 5,
            "view": [
              "XYZ",
              1,
              2,
              3
            ]
          },
          {
            "title": "abc\n",
            "page": 6,
            "view": [
              "FitR",
              1,
              2,
              3,
              4
            ]
          }
        ]
      }
    ]
  },
  {
    "title": "a",
    "page": 10,
    "view": [
      "FitB"
    ]
  }
]
EOD
); 

# default destination is [XYZ null null null]
{
    my $pdf = new_pdf ();
    pdfout_ok (
	command => ['setoutline', $pdf],
	input => '[{"title": 1,  "page": 1}]'
	);
    pdfout_ok (
	command => ['getoutline', $pdf],
	expected_out => <<'EOD'
[
  {
    "title": 1,
    "page": 1,
    "view": [
      "XYZ",
      null,
      null,
      null
    ]
  }
]
EOD
   );
} 
