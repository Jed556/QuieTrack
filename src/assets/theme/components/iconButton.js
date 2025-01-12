/*Material Dashboard 2 React base styles*/
import colors from "assets/theme/base/colors";

const { transparent } = colors;

const iconButton = {
    styleOverrides: {
        root: {
            "&:hover": {
                backgroundColor: transparent.main,
            },
        },
    },
};

export default iconButton;
