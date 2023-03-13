#include <sys/param.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/bus.h>


#include <machine/bus.h>
#include <sys/rman.h>
#include <machine/resource.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <x86/bus.h>


typedef struct test_softc *test;

struct test_softc {
  device_t testdev;
  struct cdev *test_cdev;
  int barid;
  struct resource *barres;
  bus_space_tag_t bustag;
  bus_space_handle_t bushandle;
  void *virtmem;
  u_long physmem;
  size_t sz;
  uint16_t vid;
  uint16_t pid;
};

/* Funtion Protypes */

static d_open_t test_open;
static d_close_t test_close;
static d_read_t test_read;
static d_write_t test_write;

/* Character device entry points */

static struct cdevsw test_cdevsw = {
    .d_version = D_VERSION,
    .d_open = test_open,
    .d_close = test_close,
    .d_read = test_read,
    .d_write = test_write,
    .d_name = "test",
    };


/*
 * In the cdevsw routines, we find our softc by using the si_drv1 member
 * of struct cdev. We set this variable to point to our softc in our
 * attach routine when we create the /dev entry.
 */

int test_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
  struct test_softc *sc;

  /* Look up our softc */
  
  sc = dev->si_drv1;
  device_printf(sc->testdev, "Test Device Opened\n");
  return (0);
}

int test_close(struct cdev *dev, int fflags, int devtype, struct thread *td)
{
  struct test_softc *sc;

  sc = dev->si_drv1;                                                                                          
  device_printf(sc->testdev, "Test Device Closed\n");
  return (0);                                                                                                 
}

int test_read(struct cdev *dev, struct uio *uio, int ioflag)
{
  struct test_softc *sc;

  sc = dev->si_drv1;
  device_printf(sc->testdev, "Asked to read %zd bytes.\n", uio->uio_resid);
  
  return (0);
}

int test_write(struct cdev *dev, struct uio *uio, int ioflag)
{
  struct test_softc *sc;

  sc = dev->si_drv1;
  device_printf(sc->testdev, "Asked to write %zd bytes.\n", uio->uio_resid);
  return (0);
}


static int test_probe(device_t dev){
  printf("*************PROBING****************\n");

  uint16_t vid = pci_get_vendor(dev);
  uint16_t did = pci_get_device(dev);
  printf("Test Probe\nVendor ID : 0x%x\nDevice ID : 0x%x\n",vid,did);

  if(pci_get_vendor(dev) == 0x14e4 && pci_get_device(dev) == 0x43a0){
	printf("Probe Function found The Broadcom Wifi pci_card BCM43A0.\n");
	device_set_desc(dev, "bcm43a0");
        return (BUS_PROBE_DEFAULT);
	// }
  }
  return (ENXIO);
}

static int test_attach(device_t dev){

  struct test_softc *sc;

  printf("Test pci device trying to attach to deviceID: 0x%X\n", pci_get_devid(dev));

  /* Look up our softc and initialize it's fields. */

  sc = device_get_softc(dev);
  sc->vid = 0x14e4;
  sc->pid = 0x43a0;

  
  
  sc->barid = PCIR_BAR(0);
  sc->barres = bus_alloc_resource(dev, SYS_RES_MEMORY, &sc->barid,
				       0,~0,1, RF_ACTIVE);
    if(sc->barres == NULL){
      printf("Bar resource Memory Failed to be Allocated\n");
      return (ENXIO);
    }
    device_printf(dev, "The Base Address Resource is at %p\n", sc->barres);

    //   sc->testdev = dev;
   sc->bustag = rman_get_bustag(sc->barres);
   device_printf(dev,"Bus Tag is %lu\n", sc->bustag);
   sc->bushandle = rman_get_bushandle(sc->barres);
   device_printf(dev,"Bus Handle is %lu\n", sc->bushandle);
   sc->virtmem = rman_get_virtual(sc->barres);
   device_printf(dev, "Virt Memory is at %p\n", sc->virtmem);
   sc->physmem = rman_get_start(sc->barres);
   device_printf(dev, "Physical Memory starts at %lu\n" , sc->physmem);
   sc->sz = rman_get_size(sc->barres);
   device_printf(dev, "Size of Memory is %lu\n", sc->sz);
  sc->testdev = dev;

  /*
   * Create a /dev entry for this device. the Kernel will assign us
   * a major number automatically. We use the unit number of this
   * device as the minor number and name the character device
   * "test<unit>".
   */

  sc->test_cdev = make_dev(&test_cdevsw, device_get_unit(dev),
			 UID_ROOT, GID_WHEEL, 0600, "test%u", device_get_unit(dev));
  sc->test_cdev->si_drv1 = sc;

  printf("Test device Attached!!!\n");

  return (0);
}


static int test_detach(device_t dev){
  struct test_softc *sc;

  /* teardown the state in our softc created in our attach routine. */

  sc = device_get_softc(dev);
  destroy_dev(sc->test_cdev);
  printf("Test pci device detached\n");
  return (0);
}

static int test_shutdown(device_t dev){
  printf("Test pci device shutdown\n");
    return (0);
}

static int test_suspend(device_t dev){                                                                       
  printf("Test pci device suspend\n");                                                                              
    return (0);                                                                                              
}

static int test_resume(device_t dev){
  printf("Test pci device resume\n");
  return (0);
}

static uint16_t dev_read(struct test_softc *sc, uint16_t addr){
  return bus_space_read_8(sc->bustag, sc->bushandle, addr);
}

static void dev_write(struct test_softc *sc, uint16_t addr, uint16_t value){
  bus_space_write_8(sc->bustag,sc->bushandle,addr, value);
}

static device_method_t test_methods[] = {
    DEVMETHOD(device_probe, test_probe),
    DEVMETHOD(device_attach, test_attach),
    DEVMETHOD(device_detach, test_detach),
    DEVMETHOD(device_shutdown, test_shutdown),
    DEVMETHOD(device_suspend, test_suspend),
    DEVMETHOD(device_resume, test_resume),
    DEVMETHOD_END
        };

static devclass_t test_devclass;

DEFINE_CLASS_0(test, test_driver, test_methods, sizeof(struct test_softc));
DRIVER_MODULE(test, pci, test_driver, test_devclass, 0,0);

