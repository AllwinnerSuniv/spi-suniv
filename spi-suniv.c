/*
 * Driver for the spi controller on the suniv SoC family.
 *
 * Author: IotaHydrae <writeforever@foxmail.com>
 *
 * 2023 (c) IotaHydrae.  This file is licensed under the terms of the GNU
 * General Public License version 2.  This program is licensed "as is" without
 * any warranty of any kind, whether express or implied.
 */

#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>

struct suniv_spi_regs {

};

struct suniv_spi {
    struct spi_controller       *master;
    struct spi_bitbang  bitbang;

    void __iomem        *base;
    int                 irq;

    /* used variables */

    struct clk              *hclk;
    struct clk              *mclk;
    struct reset_control    *rstc;
    struct suniv_spi_regs   reg_offsets;

    spinlock_t              lock;
    struct mutex            mutex;
    struct completion       comp;
};

struct suniv_spi_regs suniv_spi_regs_f1c100s = {

};

static bool suniv_spi_can_dma(struct spi_controller *ctlr,
					   struct spi_device *spi,
					   struct spi_transfer *xfer)
{
    return false;
}

static int suniv_spi_transfer(struct spi_device *spi, struct spi_transfer *t)
{
    return 0;
}

static void suniv_spi_set_cs(struct spi_device *spi, int is_on)
{

}

static int suniv_spi_probe(struct platform_device *pdev)
{
    struct spi_master *master;
    struct suniv_spi *sspi;
    int rc;

    pr_debug("%s, is probing ...\n", __func__);
    if (!pdev->dev.of_node)
        return -ENODEV;

    master = devm_spi_alloc_master(&pdev->dev, sizeof(*sspi));
    if (!master)
        return -ENOMEM;

    platform_set_drvdata(pdev, master);
    sspi = spi_master_get_devdata(master);

    sspi->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(sspi->base)) {
        dev_err(&pdev->dev, "Unable to map the spi address\n");
        return PTR_ERR(sspi->base);
    }

    sspi->hclk = devm_clk_get(&pdev->dev, "ahb");
    if (IS_ERR(sspi->hclk)) {
        dev_err(&pdev->dev, "Unable to acquire AHB clock\n");
        return PTR_ERR(sspi->hclk);
    }

    sspi->mclk = devm_clk_get(&pdev->dev, "mod");
    if (IS_ERR(sspi->mclk)) {
        dev_err(&pdev->dev, "Unable to acquire Module clock\n");
        return PTR_ERR(sspi->mclk);
    }

    sspi->rstc = devm_reset_control_get_exclusive(&pdev->dev, NULL);
    if (IS_ERR(sspi->rstc)) {
        dev_err(&pdev->dev, "Unable to get reset controller\n");
        return PTR_ERR(sspi->rstc);
    }

    if (of_device_is_compatible(pdev->dev.of_node, "allwinner,suniv-f1c100s-spi") ||
        of_device_is_compatible(pdev->dev.of_node, "allwinner,suniv-f1c200s-spi"))
            memcpy(&sspi->reg_offsets, &suniv_spi_regs_f1c100s, sizeof(sspi->reg_offsets));


    init_completion(&sspi->comp);
    spin_lock_init(&sspi->lock);
    mutex_init(&sspi->mutex);

    sspi->bitbang.master = master;
    sspi->bitbang.txrx_bufs = suniv_spi_transfer;
    sspi->bitbang.chipselect = suniv_spi_set_cs;

    master->can_dma = suniv_spi_can_dma;
    master->dev.of_node = pdev->dev.of_node;

#if 0
    rc = spi_register_master(master);
    if (rc) {
        dev_err(&pdev->dev, "failed to register spi master\n");
        spi_master_put(master);
        return rc;
    }
#else
    rc = spi_bitbang_start(&sspi->bitbang);
    if (rc) {
        dev_err(&pdev->dev, "failed to start spi bitbang\n");
        return rc;
    }
#endif

    return 0;
}

static int suniv_spi_remove(struct platform_device *pdev)
{
    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id suniv_spi_of_ids[] = {
    { .compatible = "allwinner,suniv-f1c100s-spi", },
    { .compatible = "allwinner,suniv-f1c200s-spi", },
    { /* KEEP THIS */ },
};
MODULE_DEVICE_TABLE(of, suniv_spi_of_ids);
#endif

static struct platform_driver suniv_spi_driver = {
    .probe   = suniv_spi_probe,
    .remove  = suniv_spi_remove,
    .driver  = {
        .name = "spi-suniv",
        .of_match_table = of_match_ptr(suniv_spi_of_ids),
    },
};

module_platform_driver(suniv_spi_driver);

MODULE_AUTHOR("IotaHydrae <writeforever@foxmail.com>");
MODULE_DESCRIPTION("Allwinner suniv SoC family spi master driver");
MODULE_LICENSE("GPL");

MODULE_ALIAS("suniv-spi");
