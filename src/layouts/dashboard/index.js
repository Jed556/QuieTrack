// @mui material components
import Grid from "@mui/material/Grid";

// Material Dashboard 2 React components
import MDBox from "components/MDBox";

// Material Dashboard 2 React example components
import { useMaterialUIController } from "context";
import ComplexStatisticsCard from "examples/Cards/StatisticsCards/ComplexStatisticsCard";
import ReportsLineChart from "examples/Charts/LineCharts/ReportsLineChart";
import DashboardLayout from "examples/LayoutContainers/DashboardLayout";
import ReportsBarChart from "examples/Charts/BarCharts/ReportsBarChart";
import DashboardNavbar from "examples/Navbars/DashboardNavbar";
import Footer from "examples/Footer";

// Configs
import configs from "config";

// Data
import reportsBarChartData from "layouts/dashboard/data/reportsBarChartData";

// Dashboard components
import { getDatabase, ref, set, onValue } from "firebase/database";
import OrdersOverview from "layouts/dashboard/components/OrdersOverview";
import Projects from "layouts/dashboard/components/Projects";
import { useState, useEffect } from "react";
import MDSnackbar from "components/MDSnackbar";

function Dashboard() {
    const [controller] = useMaterialUIController();
    const { sidenavColor } = controller;

    const [latestNoiseData, setLatestNoiseData] = useState([]);
    const [noiseData, setNoiseData] = useState([]);
    const [topFrequentValues, setTopFrequentValues] = useState([]);
    const [topFrequentCounts, setTopFrequentCounts] = useState([]);
    const [latestUpdateTime, setLatestUpdateTime] = useState("");
    const [snackbar, setSnackbar] = useState({ open: false, message: "", severity: "warning" });

    const numLatestNoiseData = 10;
    const numFrequencyData = 7;

    const handleSnackbarClose = () => {
        setSnackbar({ ...snackbar, open: false });
    };

    const showSnackbar = (message, severity) => {
        setSnackbar({ open: true, message, severity });
    };

    useEffect(() => {
        const db = getDatabase();
        const dataRef = ref(db, "noise/");
        onValue(
            dataRef,
            (snapshot) => {
                const data = snapshot.val();
                if (data) {
                    const noiseArray = Object.values(data);
                    setNoiseData(noiseArray);
                    const latestNoiseData = noiseArray.slice(-numLatestNoiseData);
                    setLatestNoiseData(latestNoiseData);

                    // Set the latest update time
                    if (latestNoiseData.length > 0) {
                        const latestTime = new Date();
                        setLatestUpdateTime(latestTime.toLocaleString());

                        // Show snackbar if the latest noise level is higher than 90 decibels
                        if (latestNoiseData[latestNoiseData.length - 1] >= 90) {
                            showSnackbar("Noise level exceeded 90 decibels!", "warning");
                        }
                    }
                } else {
                    console.log("No data found"); // Debugging line
                }
            },
            (error) => {
                console.error("Error fetching data:", error); // Debugging line
            }
        );
    }, []);

    useEffect(() => {
        const frequencyMap = noiseData.reduce((acc, value) => {
            acc[value] = (acc[value] || 0) + 1;
            return acc;
        }, {});

        const sortedFrequency = Object.entries(frequencyMap)
            .sort((a, b) => b[1] - a[1])
            .slice(0, numFrequencyData);

        setTopFrequentValues(sortedFrequency.map((item) => item[0]));
        setTopFrequentCounts(sortedFrequency.map((item) => item[1]));
    }, [noiseData]);

    const noiseChartData = {
        labels: latestNoiseData.map((_, index) => `${index + 1}`),
        datasets: {
            label: "Noise Level",
            data: latestNoiseData,
            borderColor: "rgba(75,192,192,1)",
            backgroundColor: "rgba(75,192,192,0.2)",
            fill: true,
        },
    };

    const frequencyChartData = {
        labels: topFrequentValues,
        datasets: {
            label: "Frequency",
            data: topFrequentCounts,
            borderColor: "rgba(75,192,192,1)",
            backgroundColor: "rgba(75,192,192,0.2)",
            fill: true,
        },
    };

    return (
        <DashboardLayout>
            <DashboardNavbar />
            <MDBox py={3}>
                <Grid container spacing={3}>
                    {/* <Grid item xs={12} md={6} lg={3}>
                        <MDBox mb={1.5}>
                            <ComplexStatisticsCard
                                color="dark"
                                icon="weekend"
                                title="Bookings"
                                count={281}
                                percentage={{
                                    color: "success",
                                    amount: "+55%",
                                    label: "than lask week",
                                }}
                            />
                        </MDBox>
                    </Grid>
                    <Grid item xs={12} md={6} lg={3}>
                        <MDBox mb={1.5}>
                            <ComplexStatisticsCard
                                icon="leaderboard"
                                title="Today's Users"
                                count="2,300"
                                percentage={{
                                    color: "success",
                                    amount: "+3%",
                                    label: "than last month",
                                }}
                            />
                        </MDBox>
                    </Grid>
                    <Grid item xs={12} md={6} lg={3}>
                        <MDBox mb={1.5}>
                            <ComplexStatisticsCard
                                color="success"
                                icon="store"
                                title="Revenue"
                                count="34k"
                                percentage={{
                                    color: "success",
                                    amount: "+1%",
                                    label: "than yesterday",
                                }}
                            />
                        </MDBox>
                    </Grid>
                    <Grid item xs={12} md={6} lg={3}>
                        <MDBox mb={1.5}>
                            <ComplexStatisticsCard
                                color="primary"
                                icon="person_add"
                                title="Followers"
                                count="+91"
                                percentage={{
                                    color: "success",
                                    amount: "",
                                    label: "Just updated",
                                }}
                            />
                        </MDBox>
                    </Grid> */}
                </Grid>
                <MDBox mt={4.5}>
                    <Grid container spacing={3}>
                        <Grid item xs={12} md={6} lg={4}>
                            <MDBox mb={3}>
                                <ReportsBarChart
                                    color="info"
                                    title="Top 7 Highest Noise Levels"
                                    description="Top 7 highest noise levels and their frequencies"
                                    date={latestUpdateTime}
                                    chart={frequencyChartData}
                                />
                            </MDBox>
                        </Grid>
                        <Grid item xs={12} md={6} lg={4}>
                            <MDBox mb={3}>
                                <ReportsLineChart
                                    color="success"
                                    title="Noise Levels"
                                    description={`Latest ${numLatestNoiseData} noise data points`}
                                    date={latestUpdateTime}
                                    chart={noiseChartData}
                                />
                            </MDBox>
                        </Grid>
                        {/* <Grid item xs={12} md={6} lg={4}>
                            <MDBox mb={3}>
                                <ReportsLineChart
                                    color="dark"
                                    title="completed tasks"
                                    description="Last Campaign Performance"
                                    date="just updated"
                                    chart={tasks}
                                />
                            </MDBox>
                        </Grid> */}
                    </Grid>
                </MDBox>
                {/* <MDBox>
                    <Grid container spacing={3}>
                        <Grid item xs={12} md={6} lg={8}>
                            <Projects />
                        </Grid>
                        <Grid item xs={12} md={6} lg={4}>
                            <OrdersOverview />
                        </Grid>
                    </Grid>
                </MDBox> */}
            </MDBox>
            <Footer company={configs.footer.company} />
            <MDSnackbar
                color={snackbar.severity}
                icon="notifications"
                title="Noise Alert"
                content={snackbar.message}
                open={snackbar.open}
                onClose={handleSnackbarClose}
                close={handleSnackbarClose}
                bgWhite
                anchorOrigin={{ vertical: "bottom", horizontal: "center" }}
            />
        </DashboardLayout>
    );
}

export default Dashboard;
