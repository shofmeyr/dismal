/** 
    Dismal: named after the description of economics as the "dismal science". 

    This is an implementation of a simple model where work is traded for
    money. The idea is to explore how wealth distribution can affect the
    functioning of an economy.
    
    We have a set of n agents, each of which can produce at most max_prod
    units. Each agent also consumes units, for which it pays with money. Each
    agent starts with start_money, and it can spend it employing others. An
    agent's behavior is circumscribed by how much money it has, and constraints
    on consumption: an agent will always try to consume at least min_csmp
    units, and will never consume more than max_csmp units. We always
    assume that n*max_prod >= n*min_csmp, i.e. there is always
    enough supply. Furthermore, an agent will not consume more than
    min_csmp units if it doesn't have enough savings, i.e. if money <=
    svgs. An agent will spend any amount of money > svgs, but will not
    spend more than it takes to consume max_consumption units. An agent will
    only work in an iteration (produce) if it does not have enough money to
    consume max_csmp units.

    An agent decides what price to set per production unit. Agents will start
    off all with the same price. An agent has a strategy for setting the price
    which is dependent on demand signals. We assume that the agent cannot find
    out about other agent's prices, so the agent simply sets the price based on
    how well its production units are selling. An agent has an exptd_prod
    threshold that it regards as a price trigger, i.e. if it produced units <
    exptd_prod in the previous iteration, it adjusts its price
    downwards, whereas if produced_units > exptd_prod, it adjusts its
    price upwards. The size of the adjustment is related to (produced_units -
    exptd_prod), i.e. the greater the deviation from expectation, the
    greater the adjustment.

    When an agent consumes, it will do a limited search for the best price,
    i.e. it picks k producers at random, and buys from the cheapest. It may have
    to buy from more than one producer if the producers it selects have already
    sold some of their production. This could happen, for example, when a
    previous consumer only had enough money to buy less than min_csmp
    units from a producer.

    If an agent fails to consume min_csmp units, it is below the poverty
    line. If the average production of an agent falls to zero it is regarded as
    unemployed.

    Extensions:
    ----------

    Monetary supply: we vary the amount of money in the system, i.e. we can add
    or remove money from the system. This will be like the Treasury printing
    more money. The trick will be figuring out *how* that money will get into
    the system. What we should really be doing is providing it to agents as a
    loan with interest.

    Tax system: the government collects a certain fraction of earnings as
    taxes. This could be a flat tax or a progressive tax, whatever we wish. The
    government can then use the tax in various ways: 1) it can lend it out at
    some interest rate; 2) it can have social programs where it gives money to
    the poorest agents; 3) it can use it to employ agents, i.e. the government
    uses up those agents' production units.

    Banking: agents will lend saved money to other agents at some interest
    rate. Borrowers always try to repay.

    Variable savings levels: if an agent falls below the money needed for
    minimum consumption, it adjusts its savings level upwards, so that it future
    it can save more to have more of a buffer for hard times. Conversely, if an
    agent is above its savings level, even after maximum consumption, it
    decreases the savings level, i.e. the agent has so much extra money that it
    doesn't see the need to keep such a large buffer. There would probably be
    some sort of time horizon on this, e.g. an agent will only adjust if it
    couldn't consume enough for multiple iterations.

    Evolving strategies: each agent has a randomized strategy, i.e. randomly
    selected values for the key parameters savings, exptd_prod, and the
    way it adjusts the price when not at exptd_prod. This means some
    agents will do better than others, i.e. consume more and save more. What we
    can then have is that in any iteration, an agent will randomly look at a few
    other agents, and will copy the strategy of the most successful agent in the
    sample. In this way, strategies will spread over time. 

**/

#include <stdlib.h>
#include "utils.h"
#include "cfg.h"

static FILE* _update_file;
static cfg_t _cfg;

// units of production and consumption are integers because they cannot be
// divided too finely 
typedef struct {
    int id;
    // these are all fixed for the life of the agent 
    double svgs_level;
    int max_csmp;
    int exptd_prod;
    int max_prod;
    // these fluctuate from one round to the next
    double money;
    int prod;
    int csmp;
    // total consumption over this agent's lifetime
    long tot_csmp;
    // total production over the lifetime of this agent
    long tot_prod;
    double prod_price;
    double last_price_paid;
} ag_t;

struct {
    ag_t* _;
    int num;
} _ags;

struct {
    int* _;
    int num;
} _prdrs;

struct {
    int* _;
    int num;
} _csmrs;

static int _iters = 0;

#define AG_PTR(ag_i) get_ag_ptr(ag_i, __LINE__)

#define SHOW_ROUND 0
#define SHOW_LIFETIME 1

ag_t* get_ag_ptr(int ag_i, int line_num);
void init();
void update_ag(ag_t* ag);
void compute_price(ag_t* ag);
int find_cheapest_prdr(int csmr_i);
void consume(int csmr_i, int prdr_i);
void compute_stats(int t, int show_what);
void print_ags(void);
void print_ag(ag_t* ag);

void print_array(char* label, int* array, int num) {
    printf("%s", label);
    for (int i = 0; i < num; i++) {
        printf("%4d", array[i]);
    }
    printf("\n");
}

#ifdef __APPLE__
CREATE_TIMER(MAIN_TIMER, 0);
#else
CREATE_TIMER(MAIN_TIMER);
#endif

int main(int argc, char** argv) {
    printf("DISMAL ECONOMIC MODEL (Version %.2f) compiled %s\n", 
           EMPLOY_VERSION, __DATE__);
    load_cfg(argc, argv, &_cfg);
    _update_file = fopen("updates.dat", "w");
    print_cfg(&_cfg, '#', _update_file);

    init();

    mprintf(14, "%8s", "t", 
            "%7s", "av $", "%7s", "mx $", "%7s", "mn $", 
            "%7s", "av PP", "%7s", "mx PP", "%7s", "mn PP",
            "%7s", "av C", "%7s", "mx C", "%7s", "mn C", 
            "%7s", "av P", "%7s", "mx P", "%7s", "mn P", 
            "%7s", "pvt\n");

	int iter_step = _cfg.num_iters / 50;

    timer_start(MAIN_TIMER);
    for (_iters = 0; _iters < _cfg.num_iters; _iters++) {
        _prdrs.num = 0;
        _csmrs.num = 0;

        // update the agents and setup the lists of producers and consumers
        for (int i = 0; i < _ags.num; i++) update_ag(AG_PTR(i));

        int sample = 0;
        // now try to match consumers with producers
        while (_csmrs.num && _prdrs.num) {
            // a randomly selected consumer consumes what is produced by the
            // cheapest producer in a sample
            int csmr_i = get_int_rnd(_csmrs.num);
			int prdr_i = find_cheapest_prdr(csmr_i);
			if (prdr_i != -1) consume(csmr_i, prdr_i);
        }

        // compute new prices
        for (int i = 0; i < _ags.num; i++) compute_price(AG_PTR(i));

        DBG_START(VFLAG_AGENTS) {
            print_ags();
            printf("\n");
        }

        DBG_START(VFLAG_STATS) {
            // compute and print out statistics
            if (_iters % iter_step == 0) compute_stats(_iters + 1, SHOW_ROUND);
        }
    }
    compute_stats(_iters, SHOW_LIFETIME);
	timer_stop(MAIN_TIMER);
	printf("Time taken %.2f\n", timer_read(MAIN_TIMER));

    fclose(_update_file);
}

void init() {
	init_rnd(_cfg.rseed);

    _ags.num = _cfg.num_ags;
    _ags._ = calloc(_cfg.num_ags, sizeof(ag_t));
    _prdrs._ = calloc(_cfg.num_ags, sizeof(int));
    _csmrs._ = calloc(_cfg.num_ags, sizeof(int));

    for(int i = 0; i < _ags.num; i++) {
        ag_t* ag = AG_PTR(i);
        ag->id = i;
        // note: these could be different for different ags. Initially we
        // have them all the same. We could randomly vary them too.
        ag->money = _cfg.av_money_start;
        ag->svgs_level = _cfg.av_svgs_level;
        ag->max_csmp = _cfg.av_max_csmp;
		ag->exptd_prod = _cfg.av_exptd_prod;
        ag->max_prod = _cfg.av_max_prod;
        ag->prod = ag->exptd_prod;
        ag->tot_prod = 0;
        ag->tot_csmp = 0;
		// start off by charging what we believe to be the minimum
		ag->prod_price = _cfg.min_prod_price;
		// start with enough money to buy the minimum consumption * 10
		//		ag->money = ag->prod_price * 10.0;
        // assume this starts off as the global average
        ag->last_price_paid = ag->prod_price;
    }
}

void update_ag(ag_t* ag) {
    // always start the round with no consumption
    ag->csmp = 0;
    // an agent is always a consumer if it has any money
    if (ag->money > 0) _csmrs._[_csmrs.num++] = ag->id;
    // An agent is a producer if it doesn't have enough money beyond svgs_level
    // to buy its max_csmp. To work this out, it uses the price it last
    // paid for csmp (i.e. in the previous iteration)
    if (ag->money - ag->svgs_level < ag->last_price_paid * (double)ag->max_csmp) {
        _prdrs._[_prdrs.num++] = ag->id;
        // reset production for the new round to the max 
        ag->prod = ag->max_prod;
    } else {
        ag->prod = 0;
    }
}

void compute_price(ag_t* ag) {
    // work out price for this producer based on previously expended production
    double price_change = ((double)ag->exptd_prod - (double)ag->prod) / 
		(double)ag->max_prod;
	//    ag->prod_price += (_cfg.price_adjust * price_change);
	ag->prod_price *= (1.0 + _cfg.price_adjust * price_change);
    // no agent will drop the price too low
    if (ag->prod_price < _cfg.min_prod_price) {
        ag->prod_price = _cfg.min_prod_price;
    }
}

int find_cheapest_prdr(int csmr_i) {
    // now try to find the cheapest producer in a pool of producers that is not
    // the csmr 
    double min_price = 1e9;
    int prdr_i_sel = -1;
    for (int i = 0; i < _cfg.prdr_sample_size; i++) {
        int prdr_i = get_int_rnd(_prdrs.num);
        ag_t* prdr = AG_PTR(_prdrs._[prdr_i]);
        if (prdr->id == _csmrs._[csmr_i]) continue;
        if (prdr->prod_price < min_price) {
            min_price = prdr->prod_price;
            prdr_i_sel = prdr_i;
        }
    }
    return prdr_i_sel;
}

void consume(int csmr_i, int prdr_i) {
    DBG_START(VFLAG_PC_LISTS) {
        print_array("csmrs: ", _csmrs._, _csmrs.num);
        print_array("prdrs: ", _prdrs._, _prdrs.num);
    }

    ag_t* csmr = AG_PTR(_csmrs._[csmr_i]);
    ag_t* prdr = AG_PTR(_prdrs._[prdr_i]);

    // can't cosume your own production
    if (csmr->id == prdr->id) return;

    DBG(VFLAG_CONSUME_DETAILS, "csmr->id %d, csmr->money %.2f, csmr->csmp %d, "
        "prdr->id %d, prdr->prod %d\n",
        csmr->id, csmr->money, csmr->csmp, prdr->id, prdr->prod);

    double csmr_money_avail = csmr->money;
    int min_csmp = _cfg.min_csmp - csmr->csmp;
	// this could happen if we have already consumed
	if (min_csmp < 0) min_csmp = 0;
    // always try to consume the min
    double min_spend = (double)min_csmp * prdr->prod_price;
    if (csmr_money_avail > min_spend) {
        // always spend at least the min
        csmr_money_avail = min_spend;
        if (csmr->money - min_spend > csmr->svgs_level) {
            // consumer can spend beyond the minimum
            csmr_money_avail = csmr->money - csmr->svgs_level;
            // check this should be > min_spend
            if (csmr_money_avail < min_spend) {
                FAIL("money avail %.2f < %.2f min which should never happen!\n",
                     csmr_money_avail, min_spend);
            }
            // a consumer will not consume more than the max
            double max_spend = (double)csmr->max_csmp * prdr->prod_price;
            if (csmr_money_avail > max_spend) csmr_money_avail = max_spend;
        }
    }

	// not enough money to buy at least one unit, remove from consumers list
	if (csmr_money_avail < prdr->prod_price) {
        _csmrs._[csmr_i] = _csmrs._[--_csmrs.num];
		return;
	}

    // now with the money avail, buy the max production avail
    int prod_buy = csmr_money_avail / prdr->prod_price;
    if (prod_buy > prdr->prod) prod_buy = prdr->prod;
    double money_paid = (double)prod_buy * prdr->prod_price;

    DBG(VFLAG_CONSUME_DETAILS, "min_spend %.2f, prdr price %.2f, avail money %.2f, "
        "prod_buy %d, money_paid %.2f\n", 
        min_spend, prdr->prod_price, csmr_money_avail, prod_buy, money_paid);

    prdr->prod -= prod_buy;
    prdr->tot_prod += prod_buy;
    prdr->money += money_paid;
    csmr->money -= money_paid;
    csmr->csmp += prod_buy;
    csmr->tot_csmp += prod_buy;
    csmr->last_price_paid = prdr->prod_price;
    if (prdr->prod == 0) {
        // remove from list of producers
        _prdrs._[prdr_i] = _prdrs._[--_prdrs.num];
    }
    if (csmr->csmp >= csmr->max_csmp || csmr->money == 0 || 
        (csmr->csmp >= _cfg.min_csmp && csmr->money <= csmr->svgs_level)) {
        // remove from list of consumers
        _csmrs._[csmr_i] = _csmrs._[--_csmrs.num];
    }
    
    DBG(VFLAG_CONSUME_DETAILS, "csmr money %.2f, csmr csmp %d, prdr prod %d\n",
        csmr->money, csmr->csmp, prdr->prod);

    DBG(VFLAG_CONSUME, "csmr %d, prdr %d, units %d, price %.2f\n", 
        csmr->id, prdr->id, prod_buy, money_paid);
}

void set_av_max_min(double val, double *av, double *mx, double *mn) {
    (*av) += val;
    if (*mn > val) *mn = val;
    if (*mx < val) *mx = val;
}

void compute_stats(int t, int show_what) {
    double av_money = 0;
    double min_money = 1e9;
    double max_money = 0;
    double av_csmp = 0;
    double min_csmp = 1e9;
    double max_csmp = 0;
    double av_prod = 0;
    double min_prod = 1e9;
    double max_prod = 0;
    double av_price = 0;
    double min_price = 1e9;
    double max_price = 0;
    double av_tot_prod = 0;
    double av_min_prod = 1e9;
    double av_max_prod = 0;
    double av_tot_csmp = 0;
    double av_min_csmp = 1e9;
    double av_max_csmp = 0;
    int num_in_poverty = 0;
    
    for (int i = 0; i < _ags.num; i++) {
        ag_t* ag = AG_PTR(i);
        set_av_max_min(ag->money, &av_money, &max_money, &min_money);
        set_av_max_min(ag->csmp, &av_csmp, &max_csmp, &min_csmp);
        set_av_max_min(ag->prod, &av_prod, &max_prod, &min_prod);
        set_av_max_min(ag->last_price_paid, &av_price, &max_price, &min_price);
        set_av_max_min((double)ag->tot_csmp / t, &av_tot_csmp, 
                       &av_max_csmp, &av_min_csmp);
        set_av_max_min((double)ag->tot_prod / t, &av_tot_prod, 
                       &av_max_prod, &av_min_prod);
        if (ag->csmp < _cfg.min_csmp) num_in_poverty++;
    }

    av_money /= (double)_ags.num;
    av_csmp /= (double)_ags.num;
    av_prod /= (double)_ags.num;
    av_price /= (double)_ags.num;
    av_tot_csmp /= (double)_ags.num;
    av_tot_prod /= (double)_ags.num;

	if (show_what == SHOW_LIFETIME) {
		printf(" LIFETIME\n");
		mprintf(14, "%8d", t, 
				"%7.2f", av_money, "%7.2f", max_money, "%7.2f", min_money,
				"%7.2f", av_price, "%7.2f", max_price, "%7.2f", min_price,
				"%7.2f", av_tot_csmp, "%7.2f", av_max_csmp, "%7.2f", av_min_csmp, 
				"%7.2f", av_tot_prod, "%7.2f", av_max_prod, "%7.2f", av_min_prod, 
				"%7.1f\n", (double)num_in_poverty * 100.0 / (double)_ags.num);
	} else {
		mprintf(14, "%8d", t, 
				"%7.2f", av_money, "%7.2f", max_money, "%7.2f", min_money,
				"%7.2f", av_price, "%7.2f", max_price, "%7.2f", min_price,
				"%7.2f", av_csmp, "%7.2f", max_csmp, "%7.2f", min_csmp, 
				"%7.2f", av_prod, "%7.2f", max_prod, "%7.2f", min_prod, 
				"%7.1f\n", (double)num_in_poverty * 100.0 / (double)_ags.num);
	}
}

ag_t* get_ag_ptr(int ag_i, int line_num) {
    if (ag_i < 0 || ag_i >= _ags.num) {
        FAIL("ag_i %d out of range at line %d\n", ag_i, line_num);
    }
    return &_ags._[ag_i];
}

void print_ags(void) {
    mprintf(8, "%4s", "id", "%8s", "$$", "%8s", "prod", "%8s", "csmp", "%8s", 
            "price", "%8s", "last p", "%8s", "av C", "%8s", "av P\n");
    for (int i = 0; i < _ags.num; i++) print_ag(AG_PTR(i));
}

void print_ag(ag_t* ag) {
    mprintf(8, "%4d", ag->id, "%8.2f", ag->money, "%8d", ag->prod, "%8d", 
            ag->csmp, "%8.2f", ag->prod_price, "%8.2f", ag->last_price_paid,
            "%8.2f", ag->tot_csmp / (_iters + 1), 
            "%8.2f\n", ag->tot_prod / (_iters + 1));
}
        
