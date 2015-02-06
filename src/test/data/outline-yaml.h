#define OUTLINE_YAML_10_PAGES "\
-   title: title1\n\
    page: 1\n\
    view: [XYZ, null, null, null]\n\
-   title: title 2\n\
    page: 2\n\
    view: [Fit]\n\
    open: true\n\
    kids:\n\
    -   title: 'uiax '\n\
        page: 3\n\
        view: [FitH, null]\n\
    -   title: \"mein\\nschatzzzz\"\n\
        page: 3\n\
        view: [FitH, 10]\n\
        kids:\n\
        -   title: abc♦♥♠def\n\
            page: 3\n\
            view: [FitV, null]\n\
        -   title: ♦♥♠\n\
            page: 3\n\
            view: [FitV, 20]\n\
            open: true\n\
            kids:\n\
            -   title: ♦♥♠\n\
                page: 4\n\
                view: [XYZ, null, null, 2]\n\
        -   title: ♦♥♠\n\
            page: 5\n\
            view: [XYZ, 1, 2, 3]\n\
        -   title: ♦♥♠\n\
            page: 6\n\
            view: [FitR, 1, 2, 3, 4]\n\
-   title: \n\
    page: 10\n\
    view: [XYZ, null, null, null]\n"
