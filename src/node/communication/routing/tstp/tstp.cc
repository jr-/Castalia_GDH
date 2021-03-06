#include <castalia_nic.h>

#include <tstp.h>

#include <cstring>

__USING_SYS

Define_Module (TSTP);

char TSTP::_app_id[2048];
unsigned int TSTP_Common::RADIO_RANGE;
unsigned int TSTP::Router::RADIO_RANGE;
bool TSTP_Common::drop_expired;
TSTP::Sinks TSTP::_sinks;
unsigned int TSTP::_n_sinks;
unsigned int TSTP::Life_Keeper::PERIOD;
TSTP::Frame_ID TSTP::id;
TSTP * TSTP::_instances[2048];
unsigned int TSTP::_units;

TSTP::TSTP() : _nic(0),  _unit(_units++), _locator(0), _timekeeper(0), _router(0), _security(0), _life_keeper(0), _keep_alives_sent(0), _allocated_buffers(0)
{
    trace() << "TSTP::TSTP()";
    assert(_units <= sizeof(_instances) / sizeof(TSTP *));
    _instances[_unit] = this;
    _locator = new Locator(this);
    _timekeeper = new Timekeeper(this);
    _router = new Router(this);
    _security = new Security(this);
}

TSTP::~TSTP()
{
    trace() << "TSTP::~TSTP()";
    if(_life_keeper)
        delete _life_keeper;
    if(_security)
        delete _security;
    if(_router)
        delete _router;
    if(_locator)
        delete _locator;
    if(_timekeeper)
        delete _timekeeper;
    //if(_nic)
    //    _nic->detach(this, 0);
}

void TSTP::timerFiredCallback(int timer)
{
    switch(timer) {
    case LIFEKEEPER_TIMER: {
        _life_keeper->life_keeper();
    } break;
    case KEY_MANAGER_TIMER: {
        _security->key_manager();
    } break;
    case AUTH_REQUEST_TIMER: {
        _security->auth_request();
    } break;
    }
}

void TSTP::startup()
{
    trace() << "TSTP::startup()";
    _sink = par("sink");
    _expiry = par("expiry").doubleValue();
    _security_enabled = par("securityEnabled");
    TSTP_Common::RADIO_RANGE = par("radioRange");
    TSTP_Common::drop_expired = par("dropExpired");
    trace() << "I am a " << (_sink ? "sink" : "sensor");
}

void TSTP::fromMacLayer(cPacket * pkt, int a, double b, double c)
{
    if(!strcmp(pkt->getName(), "TSTP pointer setup")) {
        _nic = reinterpret_cast<NIC *>(pkt->getBitLength());
        _life_keeper = new Life_Keeper(this);

        _locator->bootstrap();
        _timekeeper->bootstrap();
        _router->bootstrap();
        if(_security_enabled)
            _security->bootstrap();

        _nic->attach(this, NIC::TSTP);

        trace() << "TSTP = " << this << ", NIC = " << pkt->getBitLength();
    } else if(!strcmp(pkt->getName(), "TSTP data hook")) {
        TSTPPacket * p = check_and_cast<TSTPPacket *>(pkt);
        toApplicationLayer(p->decapsulate());
    }
}

void TSTP::Locator::bootstrap()
{
    VirtualMobilityManager *nodeMobilityModule = check_and_cast<
                VirtualMobilityManager*>(
                _tstp->getParentModule()->getParentModule()->getParentModule()->getSubmodule(
                    "node", _tstp->self)->getSubmodule("MobilityManager"));

    Coordinates::Number x = nodeMobilityModule->getLocation().x * 100.0;
    Coordinates::Number y = nodeMobilityModule->getLocation().y * 100.0;
    Coordinates::Number z = nodeMobilityModule->getLocation().z * 100.0;

    _here = Coordinates(x, y, z);

    if(_tstp->_sink)
        _tstp->_sinks.insert(new Sinks::Element(&_here), _tstp->_n_sinks++);

    _tstp->trace() << "_here = " << _here;

    _confidence = 100;

    _tstp->_nic->attach(this, NIC::TSTP);
}

void TSTP::Timekeeper::bootstrap()
{
    SYNC_PERIOD = _tstp->par("ptpExplicitSyncPeriod").doubleValue();
    _adjusts = new cOutVector("Timer adjusts");

    bool start_synchronized = _tstp->par("startSynchronized");

    _disable_explicit_sync = !_tstp->par("usePtpExplicitSync");
    if(_tstp->_sink)
        _next_sync = -1ull; // Just so that the sink will always have synchronized() return true
    else if(start_synchronized)
        _next_sync = now() + SYNC_PERIOD;
    else
        _next_sync = 0;

    _tstp->_nic->attach(this, NIC::TSTP);
}

void TSTP::Router::bootstrap()
{
    expiry_metric = _tstp->par("useExpiryMetric");
    old_expiry_metric = _tstp->par("useOldExpiryMetric");
    effort_metric = _tstp->par("useEffortMetric");
    random_metric = _tstp->par("useRandomMetric");
    RADIO_RANGE = TSTP_Common::RADIO_RANGE;
    EXPIRY_BETHA = _tstp->par("expiryBetha").doubleValue() * 1000000.0;

    _tstp->_nic->attach(this, NIC::TSTP);
}

void TSTP::Security::bootstrap()
{
    assert(_tstp->_sinks.size() == 1); // Security only works in single-sink mode for now

    KEY_EXPIRY = _tstp->par("keyExpiry").doubleValue();
    DHREQUEST_AUTH_DELAY = _tstp->par("dhRequestProcessingDelay").doubleValue();
    KEY_MANAGER_PERIOD = _tstp->par("keyManagerPeriod").doubleValue();
    POLY_TIME_WINDOW = _tstp->par("securityTimeWindow").doubleValue();

    _tstp->_nic->attach(this, NIC::TSTP);

    if(_tstp->_units < 2) {
        if(_tstp->_sink) {
                for(unsigned int i = 0; i < _tstp->_units; i++) {
                    if(i == _tstp->_unit)
                        continue;
                    bool send_messages = !_tstp->_instances[i]->par("startWithKeysEstablished");
                    if(send_messages) {
                        // Bootstrap might not have been called yet at the other nodes
                        VirtualMobilityManager *nodeMobilityModule = check_and_cast<
                                    VirtualMobilityManager*>(
                                    _tstp->getParentModule()->getParentModule()->getParentModule()->getSubmodule(
                                        "node", i)->getSubmodule("MobilityManager"));

                        Coordinates::Number x = nodeMobilityModule->getLocation().x * 100.0;
                        Coordinates::Number y = nodeMobilityModule->getLocation().y * 100.0;
                        Coordinates::Number z = nodeMobilityModule->getLocation().z * 100.0;

                        Coordinates there(x, y, z);

                        add_peer(_tstp->_instances[i]->_security->_id, sizeof(Node_ID), Region(there, 0, 0, -1));
                    }
                }
            } else {
                if(_tstp->par("startWithKeysEstablished")) {

                    // Establish keys without sending any messages

                    TSTP::Security * sink;
                    for(unsigned int i = 0; i < _tstp->_units; i++) {
                        if(_tstp->_instances[i]->_sink) {
                            sink = _tstp->_instances[i]->_security;
                            break;
                        }
                    }
                    Master_Secret ms = _dh.shared_key(sink->_dh.public_key());
                    Master_Secret ms2 = sink->_dh.shared_key(_dh.public_key());
                    assert(ms == ms2);
                    Peer * p = new Peer(_id, Region(_tstp->sink(), 0, 0, -1), _tstp);
                    Peer * p2 = new Peer(_id, Region(_tstp->here(), 0, 0, -1), sink->_tstp);
                    p->master_secret(ms);
                    p2->master_secret(ms2);
                    _trusted_peers.insert(p->link());
                    sink->_trusted_peers.insert(p2->link());
                } else {
                    Peer * peer = new Peer(_id, Region(_tstp->sink(), 0, 0, -1), _tstp);
                    _pending_peers.insert(peer->link());
                }
            }

        } else {
            if(_tstp->_sink) {
                typedef TSTP::Region::Space Space;
                auto nodes = Simple_List<Space>();
                //add nodes em uma lista
                for(unsigned int i = 0; i < _tstp->_units; i++) {
                    if(i == _tstp->_unit)
                        continue;
                    bool send_messages = !_tstp->_instances[i]->par("startWithKeysEstablished");
                    if(send_messages) {
                        // Bootstrap might not have been called yet at the other nodes
                        VirtualMobilityManager *nodeMobilityModule = check_and_cast<
                                    VirtualMobilityManager*>(
                                    _tstp->getParentModule()->getParentModule()->getParentModule()->getSubmodule(
                                        "node", i)->getSubmodule("MobilityManager"));

                        Coordinates::Number x = nodeMobilityModule->getLocation().x * 100.0;
                        Coordinates::Number y = nodeMobilityModule->getLocation().y * 100.0;
                        Coordinates::Number z = nodeMobilityModule->getLocation().z * 100.0;

                        Space node(Space::Center(x,y,z));
                        List_Elements::Singly_Linked<Space> el_node(&node);
                        nodes.insert(&el_node);
                    }
                }
                _tstp->trace() << "TSTP::GDH_Security::begin_group_diffie_hellman()" << endl;

                Group_Id group_id = 1;
                _gdh = Group_Diffie_Hellman();
                Parameters params = _gdh.parameters();
                Region::Space *last = nodes.remove_tail()->object();
                auto gw = Region::Space(_tstp->here());

                Buffer* resp = _tstp->alloc(sizeof(GDH_Setup_Last));
                new (resp->frame()) GDH_Setup_Last(group_id, *last, params, gw);
                _tstp->marshal(resp);
                //kout << "Sending Setup Last to " << *last << " with next: " << gw << endl;
                _tstp->_nic->send(resp);

                Region::Space* first = nodes.remove_head()->object();

                for(auto it = nodes.begin(); it; it++) {
                    resp = _tstp->alloc(sizeof(GDH_Setup_Last_Follow));
                    new (resp->frame()) GDH_Setup_Last_Follow(group_id, *last, *it->object(), false);
                    _tstp->marshal(resp);
                    //kout << "Sending GDH Setup Last Follow to "<< *last << " with next = "<< *it->object() << ", " << false << endl;
                    _tstp->_nic->send(resp);
                }

                resp = _tstp->alloc(sizeof(GDH_Setup_Last_Follow));
                new (resp->frame()) GDH_Setup_Last_Follow(group_id, *last, *first, true);
                _tstp->marshal(resp);
                //kout << "Sending GDH Setup Last Follow to "<< *last << " with next = "<< *first << ", " << true << endl;
                _tstp->_nic->send(resp);

                Region::Space* next = last;
                if(nodes.size() > 0) {
                    for(auto it = nodes.begin(); it; it++) {
                        resp = _tstp->alloc(sizeof(GDH_Setup_Intermediate));
                        Region::Space *current = it->object();
                        new (resp->frame()) GDH_Setup_Intermediate(group_id, *current, params, *next);
                        _tstp->marshal(resp);
                        //kout << "Sending Setup Intermediate to " << *current << endl;
                        _tstp->_nic->send(resp);
                        next = current;
                    }
                }

                Region::Space* firsts_next = nodes.head()->object();

                resp = _tstp->alloc(sizeof(GDH_Setup_First));
                new (resp->frame()) GDH_Setup_First(group_id, *first, params, *firsts_next);
                _tstp->marshal(resp);
                //kout << "Sending Setup First to " << *first << endl;
                _tstp->_nic->send(resp);

                _GDH_state = GDH_WAITING_GW;

            }
        }

}

void TSTP::bootstrap()
{
    trace() << "TSTP::bootstrap()";
    _nic->attach(this, NIC::TSTP);
}

void TSTP::finishSpecific()
{
    recordScalar("Here x", here().x);
    recordScalar("Here y", here().y);
    recordScalar("Here z", here().z);
    recordScalar("Using Effort Metric", _router->effort_metric);
    recordScalar("Using Old Expiry Metric", _router->old_expiry_metric);
    recordScalar("Using Expiry Metric", _router->expiry_metric);
    recordScalar("Using Random Metric", _router->random_metric);

    recordScalar("Keep Alives Sent", _keep_alives_sent);
    recordScalar("Number of allocated buffers", _allocated_buffers);

    TimerService::recordTimerDrift();

    recordScalar("Number of timer adjustments", _timekeeper->_n_adjusts);
    recordScalar("Total timer error (timestamp)", _timekeeper->_total_error);
    recordScalar("Total timer error (us)", _nic->count2us(_timekeeper->_total_error));
    recordScalar("Max timer error (timestamp)", _timekeeper->_max_error);
    recordScalar("Max timer error (us)", _nic->count2us(_timekeeper->_max_error));

    if(_timekeeper->_n_adjusts) {
        recordScalar("Average timer error (timestamp)", (double)(_timekeeper->_total_error) / (double)(_timekeeper->_n_adjusts));
        recordScalar("Average timer error (us)", (double)(_nic->count2us(_timekeeper->_total_error)) / (double)(_timekeeper->_n_adjusts));
    }
}

void TSTP::fromApplicationLayer(cPacket * pkt, const char * destination)
{
    ApplicationPacket * appPkt = dynamic_cast<ApplicationPacket *>(pkt);
    if(appPkt == NULL)
        return;

    strcpy(_app_id, appPkt->getApplicationID());

    Buffer * buf = alloc(sizeof(Response));
    Response * resp = new (buf->frame()->data<Response>()) Response(0, 0, _expiry);
    resp->time(now());
    resp->origin(here());
    *(resp->data<uint32>()) = appPkt->getData();
//    id++;
//    id = id & Microframe::ID_MAX;
//    *(resp->data<uint32>()) = id;
    resp->expiry(now() + _expiry);
    buf->size(sizeof(Response) - sizeof(Response::Data) + sizeof(uint32));
    marshal(buf);

    _nic->send(buf);

    cancelAndDelete(pkt);
}

bool TSTP::notify(void * subject, Buffer * buf)
{
    static unsigned int seq_num = 0;
    trace() << "TSTP::notify(s=" << subject << ",b=" << buf << ")";
    if(_sink) {
        if((subject == 0) || (!_observed.notify(int64(subject), buf))) {
            Header * h = buf->frame()->data<Header>();
            bool found = false;
            for(auto it = _last_message.begin(); it != _last_message.end(); ++it) {
                if((*it).first == h->origin()) {
                    found = true;
                    if ((*it).second < h->time()) {
                        (*it).second = h->time();
                        break;
                    }
                    else
                        return false;
                }
            }
            if(!found)
                _last_message.push_back(std::make_pair(h->origin(), h->time()));

            ApplicationPacket * newPacket = new ApplicationPacket(
                "TSTP data hook", APPLICATION_PACKET);
            newPacket->setApplicationID(_app_id);
            newPacket->setData(*buf->frame()->data<Response>()->data<uint32>());
            newPacket->setSequenceNumber(seq_num++);
            newPacket->setByteLength(sizeof(uint32));
            TSTPPacket * tp = new TSTPPacket("TSTP data hook",
                                             NETWORK_LAYER_PACKET);
            AppNetInfoExchange_type info = newPacket->getAppNetInfoExchange();
            info.timestamp = simtime_t(h->time(), SIMTIME_US);
            newPacket->setAppNetInfoExchange(info);
            tp->encapsulate(newPacket);
            _nic->mac2net(tp); // Castalia thinks we are still in the MAC layer
        }
    }

    return true;
}

// TSTP::Locator
// Class attributes


// Methods
void TSTP::Locator::update(NIC::Observed * obs, NIC::Protocol prot, Buffer * buf)
{
    _tstp->trace() << "TSTP::Locator::update(obs=" << obs << ",buf=" << buf << ")" << endl;
    if(buf->is_microframe) {
        buf->sender_distance = buf->hint; // This would fit better in the Router, but Timekeeper uses this info
        if(!synchronized())
            buf->relevant = true;
        else if(!buf->downlink)
            buf->my_distance = here() - TSTP::sink();
    } else {
        Coordinates dst = _tstp->destination(buf).center;
        buf->sender_distance = buf->frame()->data<Header>()->last_hop() - dst;
        if(_confidence < 100) {
            Header * h = buf->frame()->data<Header>();
            if(h->confidence() > 80)
                add_peer(h->last_hop(), h->confidence(), buf->rssi);
        }

        buf->my_distance = here() - dst;
        buf->downlink = dst != TSTP::sink(); // This would fit better in the Router, but Timekeeper uses this info

        // Respond to Keep Alive if sender is low on location confidence
        if(synchronized()) {
            Header * header = buf->frame()->data<Header>();
            if(header->type() == CONTROL) {
                Control * control = buf->frame()->data<Control>();
                if((control->subtype() == KEEP_ALIVE) && (header->confidence() < 80))
                    _tstp->keep_alive();
            }
        }
    }
}

void TSTP::Locator::marshal(Buffer * buf)
{
    _tstp->trace() << "TSTP::Locator::marshal(buf=" << buf << ")" << endl;
    Coordinates dst = _tstp->destination(buf).center;
    buf->my_distance = here() - dst;
    if(buf->is_new)
        buf->sender_distance = buf->my_distance;
    buf->downlink = dst != TSTP::sink(); // This would fit better in the Router, but Timekeeper uses this info
    buf->frame()->data<Header>()->confidence(_confidence);
    Coordinates here = _tstp->here();
    buf->frame()->data<Header>()->origin(here);
    buf->frame()->data<Header>()->last_hop(here);
}

TSTP::Locator::~Locator()
{
    _tstp->trace() << "TSTP::~Locator()" << endl;
    if(_tstp->_sink) {
        Sinks::Element * el;
        el = _tstp->_sinks.search(&_here);
        if(el) {
            _tstp->_sinks.remove(el);
            _tstp->_n_sinks--;
            delete el;
        }
    }
    //_tstp->_nic->detach(this, 0);
}

// TSTP::Timekeeper
// Class attributes


// Methods
void TSTP::Timekeeper::update(NIC::Observed * obs, NIC::Protocol prot, Buffer * buf)
{
    _tstp->trace() << "TSTP::Timekeeper::update(obs=" << obs << ",buf=" << buf << ")" << endl;

    if(buf->is_microframe) {
        if(!synchronized())
            buf->relevant = true;
    } else {
        buf->deadline = _tstp->destination(buf).t1;

        Header * header = buf->frame()->data<Header>();

        if(header->time_request()) {
            _tstp->trace() << "TSTP::Timekeeper::update: time_request received" << endl;
            // Respond to Time Request if able
            if(synchronized()) {
                bool peer_closer_to_sink = buf->downlink ?
                    (_tstp->here() - _tstp->sink() > header->last_hop() - _tstp->sink()) :
                    (buf->my_distance > buf->sender_distance);

                if(!peer_closer_to_sink) {
                    _tstp->trace() << "TSTP::Timekeeper::update: responding to time request" << endl;
                    _tstp->keep_alive();
                }
            }
        } else {
            if(_t1 == 0) { // No peer
                bool peer_closer_to_sink = buf->downlink ?
                    (_tstp->here() - _tstp->sink() > header->last_hop() - _tstp->sink()) :
                    (buf->my_distance > buf->sender_distance);

                if(peer_closer_to_sink) {
                    Time_Stamp t0 = header->last_hop_time() + Radio::Timer::us2count(IEEE802_15_4::PHY_HEADER_SIZE * 1000000 / IEEE802_15_4::BYTE_RATE);
                    Time_Stamp t1 = buf->sfd_time_stamp;

                    _next_sync = now();

                    _t0 = t0;
                    _t1 = t1;

                    Offset off = t0 - t1;
                    _tstp->_nic->adjust(off);

                    _peer = header->last_hop();

                    _tstp->trace() << "TSTP::Timekeeper::update: synchronizing with " << _peer << endl;
                }
            } else if(_peer == header->last_hop()) { // Message from peer
                Time_Stamp t2 = header->last_hop_time() + Radio::Timer::us2count(IEEE802_15_4::PHY_HEADER_SIZE * 1000000 / IEEE802_15_4::BYTE_RATE);
                Time_Stamp t3 = buf->sfd_time_stamp;

                Offset off = t2 - t3;
                _tstp->_nic->adjust(off);
                /*
                _t1 = _tstp->_nic->adjust(_t0, _t1, t2, t3);
                _t0 = t2;

//                if(abs(_tstp->_nic->_instance[0]->Timer::count2us(_tstp->_nic->_instance[0]->Timer::read()) - _tstp->_nic->Timer::count2us(_tstp->_nic->Timer::read())) > 1) {
//                    std::cout << _tstp->_nic->_instance[0]->Timer::count2us(_tstp->_nic->_instance[0]->Timer::read()) << std::endl;
//                    std::cout << _tstp->_nic->Timer::count2us(_tstp->_nic->Timer::read()) << std::endl;
//                    throw cRuntimeError("Time sync accuracy error!");
//                }
  */

                _next_sync = now() + SYNC_PERIOD; // TODO

                //if(!_first_pass) {
                    //_total_error += abs(adj * 1000000ull / static_cast<long long>(t0_new - _t0)); // in ppm
                    Offset error = abs(off);
                    //_adjusts->record(off);
                    _adjusts->record(error);
                    if(error > _max_error)
                        _max_error = error;
                    _total_error += error;
                    _n_adjusts++;
                //}

                _first_pass = false;

                _tstp->trace() << "TSTP::Timekeeper::update: timer offset of " << off << " corrected." << endl;
                _tstp->trace() << "now() = " << now() << endl;
            }
        }
    }
}

void TSTP::Timekeeper::marshal(Buffer * buf)
{
    _tstp->trace() << "TSTP::Timekeeper::marshal(buf=" << buf << ")" << endl;
    buf->frame()->data<Header>()->time_request(!synchronized());
    buf->frame()->data<Header>()->time(now());
    buf->deadline = _tstp->destination(buf).t1; // deadline must be set after origin time for Security messages
}

TSTP::Timekeeper::~Timekeeper()
{
    _tstp->trace() << "TSTP::~Timekeeper()" << endl;
    delete _adjusts;
    //_tstp->_nic->detach(this, 0);
}

// TSTP::Router
// Class attributes

// Methods
void TSTP::Router::update(NIC::Observed * obs, NIC::Protocol prot, Buffer * buf)
{
    _tstp->trace() << "TSTP::Router::update(obs=" << obs << ",buf=" << buf << ")" << endl;
    if(buf->is_microframe) {
        buf->progress_bits = buf->my_distance < buf->sender_distance;
        buf->relevant |= buf->progress_bits;
    } else {
        Header * header = buf->frame()->data<Header>();
        // Keep Alive messages are never forwarded
        if((header->type() == CONTROL) && (buf->frame()->data<Control>()->subtype() == KEEP_ALIVE))
            buf->destined_to_me = false;
        else {
            Region dst = _tstp->destination(buf);
            buf->destined_to_me = ((header->origin() != _tstp->here()) && (dst.contains(_tstp->here(), dst.t0)));

            if(forward(buf)) {

                // Forward or ACK the message

                Buffer * send_buf = _tstp->alloc(buf->size());

                // Copy frame contents
                memcpy(send_buf->frame(), buf->frame(), buf->size());

                // Copy Buffer Metainformation
                send_buf->size(buf->size());
                send_buf->id = buf->id;
                send_buf->destined_to_me = buf->destined_to_me;
                send_buf->downlink = buf->downlink;
                send_buf->deadline = buf->deadline;
                send_buf->my_distance = buf->my_distance;
                send_buf->sender_distance = buf->sender_distance;
                send_buf->is_new = false;
                send_buf->is_microframe = false;
                send_buf->random_backoff_exponent = 0;
                send_buf->progress_bits = buf->progress_bits;

                // Calculate offset
                offset(send_buf);

                // Adjust Last Hop location
                Header * header = send_buf->frame()->data<Header>();
                header->last_hop(_tstp->here());
                send_buf->sender_distance = send_buf->my_distance;

                header->confidence(_tstp->_locator->_confidence);
                header->time_request(!_tstp->_timekeeper->synchronized());

                send_buf->hint = send_buf->my_distance;

                _tstp->_nic->send(send_buf);
            }
        }
    }
}

void TSTP::Router::marshal(Buffer * buf)
{
    _tstp->trace() << "TSTP::Router::marshal(buf=" << buf << ")" << endl;
    TSTP::Region dest = _tstp->destination(buf);
    buf->downlink = dest.center != _tstp->sink();
    buf->destined_to_me = (buf->frame()->data<Header>()->origin() != _tstp->here()) && (dest.contains(_tstp->here(), _tstp->now()));
    buf->hint = buf->my_distance;
    buf->progress_bits = 1;

    offset(buf);
}

TSTP::Router::~Router()
{
    _tstp->trace() << "TSTP::~Router()" << endl;
    //_tstp->_nic->detach(this, 0);
}

// TSTP::Security
// Class attributes

// Methods
void TSTP::Security::update(NIC::Observed * obs, NIC::Protocol prot, Buffer * buf)
{
    bool was_bootstrapped = bootstrapped();
    if(_tstp->_units < 2) {

        _tstp->trace() << "TSTP::Security::update(obs=" << obs << ",buf=" << buf << ")" << endl;

        if(!buf->is_microframe && buf->destined_to_me) {

            switch(buf->frame()->data<Header>()->type()) {

                case CONTROL: {
                    _tstp->trace() << "TSTP::Security::update(): Control message received" << endl;
                    switch(buf->frame()->data<Control>()->subtype()) {

                        case DH_REQUEST: {
                            if(_tstp->here() != _tstp->sink()) {
                                DH_Request * dh_req = buf->frame()->data<DH_Request>();
                                _tstp->trace() << "TSTP::Security::update(): DH_Request message received: " << *dh_req << endl;

                                //while(CPU::tsl(_peers_lock));
                                //CPU::int_disable();
                                bool valid_peer = false;
                                for(Peers::Element * el = _pending_peers.head(); el; el = el->next())
                                    if(el->object()->valid_deploy(dh_req->origin(), _tstp->now())) {
                                        valid_peer = true;
                                        _tstp->trace() << "Found pending peer";
                                        break;
                                    }
                                if(!valid_peer)
                                    for(Peers::Element * el = _trusted_peers.head(); el; el = el->next())
                                        if(el->object()->valid_deploy(dh_req->origin(), _tstp->now())) {
                                            valid_peer = true;
                                            _trusted_peers.remove(el);
                                            _pending_peers.insert(el);
                                            _tstp->trace() << "Moving trusted peer to pending";
                                            break;
                                        }
                                //_peers_lock = false;
                                //CPU::int_enable();

                                if(valid_peer) {
                                    _tstp->trace() << "TSTP::Security::update(): Sending DH_Response" << endl;
                                    // Respond to Diffie-Hellman request
                                    Buffer * resp = _tstp->alloc(sizeof(DH_Response));
                                    new (resp->frame()) DH_Response(_dh.public_key());
                                    _tstp->marshal(resp);
                                    _tstp->_nic->send(resp);

                                    // Calculate Master Secret
                                    Pending_Key * pk = new Pending_Key(buf->frame()->data<DH_Request>()->key(), _tstp);
                                    pk->master_secret();
                                    //Master_Secret ms = pk->master_secret();
                                    //while(CPU::tsl(_peers_lock));
                                    //CPU::int_disable();
                                    _pending_keys.insert(pk->link());
                                    //_peers_lock = false;
                                    //CPU::int_enable();

                                    _tstp->schedule_auth_request(); // Castalia workaround
                                }
                            }
                        } break;

                        case DH_RESPONSE: {
                            if(_dh_requests_open) {
                                DH_Response * dh_resp = buf->frame()->data<DH_Response>();
                                _tstp->trace() << "TSTP::Security::update(): DH_Response message received: " << *dh_resp << endl;

                                //CPU::int_disable();
                                bool valid_peer = false;
                                for(Peers::Element * el = _pending_peers.head(); el; el = el->next())
                                    if(el->object()->valid_deploy(dh_resp->origin(), _tstp->now())) {
                                        valid_peer = true;
                                        _tstp->trace() << "Valid peer found: " << *el->object() << endl;
                                        break;
                                    }

                                if(valid_peer) {
                                    _dh_requests_open--;
                                    Pending_Key * pk = new Pending_Key(buf->frame()->data<DH_Response>()->key(), _tstp);
                                    _pending_keys.insert(pk->link());
                                    _tstp->trace() << "TSTP::Security::update(): Inserting new Pending Key: " << *pk << endl;
                                }
                                //CPU::int_enable();
                            }
                        } break;

                        case AUTH_REQUEST: {

                            Auth_Request * auth_req = buf->frame()->data<Auth_Request>();
                            _tstp->trace() << "TSTP::Security::update(): Auth_Request message received: " << *auth_req << endl;

                            //CPU::int_disable();
                            Peer * auth_peer = 0;
                            for(Peers::Element * el = _pending_peers.head(); el; el = el->next()) {
                                Peer * peer = el->object();

                                if(peer->valid_request(auth_req->auth(), auth_req->origin(), _tstp->now())) {
                                    _tstp->trace() << "valid_request " << endl;
                                    for(Pending_Keys::Element * pk_el = _pending_keys.head(); pk_el; pk_el = pk_el->next()) {
                                        Pending_Key * pk = pk_el->object();
                                        if(verify_auth_request(pk->master_secret(), peer->id(), auth_req->otp())) {
                                            peer->master_secret(pk->master_secret());
                                            _pending_peers.remove(el);
                                            _trusted_peers.insert(el);
                                            auth_peer = peer;

                                            _pending_keys.remove(pk_el);
                                            delete pk_el->object();

                                            break;
                                        }
                                    }
                                    if(auth_peer)
                                        break;
                                }
                            }
                            //CPU::int_enable();

                            if(auth_peer) {
                                Node_Auth encrypted_auth;
                                encrypt(auth_peer->auth(), auth_peer, encrypted_auth);

                                Buffer * resp = _tstp->alloc(sizeof(Auth_Granted));
                                new (resp->frame()) Auth_Granted(Region::Space(auth_peer->valid().center, auth_peer->valid().radius), encrypted_auth);
                                _tstp->marshal(resp);
                                _tstp->trace() << "TSTP::Security: Sending Auth_Granted message " << resp->frame()->data<Auth_Granted>() << endl;
                                _tstp->_nic->send(resp);
                            } else
                                _tstp->trace() << "TSTP::Security::update(): No peer found" << endl;
                        } break;

                        case AUTH_GRANTED: {

                            if(_tstp->here() != _tstp->sink()) {
                                Auth_Granted * auth_grant = buf->frame()->data<Auth_Granted>();
                                _tstp->trace() << "TSTP::Security::update(): Auth_Granted message received: " << *auth_grant << endl;
                                //CPU::int_disable();
                                bool auth_peer = false;
                                for(Peers::Element * el = _pending_peers.head(); el; el = el->next()) {
                                    Peer * peer = el->object();
                                    for(Pending_Keys::Element * pk_el = _pending_keys.head(); pk_el; pk_el = pk_el->next()) {
                                        Pending_Key * pk = pk_el->object();
                                        Node_Auth decrypted_auth;
                                        OTP key = otp(pk->master_secret(), peer->id());
                                        _cipher.decrypt(auth_grant->auth(), key, decrypted_auth);
                                        if(decrypted_auth == _auth) {
                                            peer->master_secret(pk->master_secret());
                                            _pending_peers.remove(el);
                                            _trusted_peers.insert(el);
                                            auth_peer = true;
                                            _pending_keys.remove(pk_el);
                                            delete pk_el->object();

                                            break;
                                        }
                                    }
                                    if(auth_peer)
                                        break;
                                }
                                if(auth_peer) {
                                    _tstp->trace() << "Auth Granted!" << endl;
                                    _bootstrapped = true;
                                }
                                else
                                    _tstp->trace() << "No peer found for Auth code" << endl;
                                //CPU::int_enable();
                            }
                        } break;

                        default: break;
                    }
                } break;
                case RESPONSE: {
                    _tstp->trace() << "TSTP::Security::update(): Response message received" << endl;
                    _tstp->trace() << "from " << buf->frame()->data<Header>()->origin() << endl;
                    Response * resp = buf->frame()->data<Response>();
                    Time reception_time = _tstp->_nic->count2us(buf->sfd_time_stamp);
                    for(Peers::Element * el = _trusted_peers.head(); el; el = el->next()) {
                        if(el->object()->valid_deploy(buf->frame()->data<Header>()->origin(), _tstp->now())) {
                            _tstp->trace() << "Found valid peer" << endl;
                            unsigned char * data = resp->data<unsigned char>();
                            if(unpack(el->object(), data, &data[sizeof(Master_Secret)], reception_time)) {
                                buf->trusted = true;
                                _tstp->trace() << "Upack done!" << endl;
                                break;
                            } else {
                                _tstp->trace() << "Unpack failed" << endl;
                            }
                        }
                    }
                } break;

                default: break;
            }
        } else
            buf->trusted = true;


    } else {
        _tstp->trace() << "TSTP::GDH_Security::update(obs=" << obs << ",buf=" << buf << ")" << endl;

        if(buf->is_microframe || !buf->destined_to_me || buf->frame()->data<Header>()->type() != CONTROL){
            return; //we dont care if the this is the type of the message
        }

        _tstp->trace() << "TSTP::GDH_Security::update(): Control message received" << endl;
        /*kout << "TSTP::GDH_Security::update(): Control message received" << endl;
        kout << "Message type: " << buf->frame()->data<Control>()->subtype() << endl;*/

        auto author = buf->frame()->data<Header>()->origin();

        switch(buf->frame()->data<Control>()->subtype()) {
            case GDH_SETUP_FIRST: {
                if(_tstp->here() != _tstp->sink()) {
                    _tstp->trace() << "TSTP::GDH_Security::update(): GDH SETUP FIRST message received" << endl;
                    //kout << "TSTP::GDH_Security::update(): GDH SETUP FIRST message received from "  << author << endl;
                    GDH_Setup_First* message = buf->frame()->data<GDH_Setup_First>();
                    Region::Space next = message->next();
                    _gdh = Group_Diffie_Hellman(message->parameters());
                    Round_Key round_key = _gdh.insert_key();
                    //uses the randomly generated private key in the GDH object creation
                    _GDH_node_type = GDH_FIRST;
                    _GDH_state = GDH_WAITING_POP;
                    //this node is waiting to remove its key from the round key
                    Buffer* resp = _tstp->alloc(sizeof(GDH_Round));
                    new (resp->frame()) GDH_Round(message->group_id(), next, round_key);
                    _tstp->marshal(resp);
                    _tstp->_nic->send(resp);
                }
            } break;
            case GDH_SETUP_INTERMEDIATE: {
                if(_tstp->here() != _tstp->sink()) {
                    _tstp->trace() << "TSTP::GDH_Security::update(): GDH SETUP INTERMEDIATE message received" << endl;
                    //kout << "TSTP::GDH_Security::update(): GDH SETUP INTERMEDIATE message received from " << author << endl;
                    GDH_Setup_Intermediate* message = buf->frame()->data<GDH_Setup_Intermediate>();
                    _gdh = Group_Diffie_Hellman(message->parameters());
                    _GDH_next = Simple_List<Region::Space>(); //only correct for the first group id
                    List_Elements::Singly_Linked<Region::Space> *next = new List_Elements::Singly_Linked<Region::Space>(new Region::Space(message->next()));
                    _GDH_next.insert(next);
                    _GDH_node_type = GDH_INTERMEDIATE;
                    _GDH_state = GDH_WAITING_EXP; //this node is waiting to exponentiate the round key
                }
            } break;
            case GDH_SETUP_LAST: {
                if(_tstp->here() != _tstp->sink()) {
                    _tstp->trace() << "TSTP::GDH_Security::update(): GDH SETUP LAST message received" << endl;
                    GDH_Setup_Last* message = buf->frame()->data<GDH_Setup_Last>();
                    //kout << "TSTP::GDH_Security::update(): GDH SETUP LAST message received from " << author << " with next = " << message->next() << endl;
                    List_Elements::Singly_Linked<Region::Space> *next = new List_Elements::Singly_Linked<Region::Space>(new Region::Space(message->next()));
                    _GDH_next.insert(next);
                    _gdh = Group_Diffie_Hellman(message->parameters());
                    _GDH_node_type = GDH_LAST;
                    _GDH_state = GDH_WAITING_NEXT; //this node is waiting to receive all his next nodes
                }
            } break;
            case GDH_SETUP_LAST_FOLLOW: {
                if(_tstp->here() != _tstp->sink() && _GDH_node_type == GDH_LAST && _GDH_state == GDH_WAITING_NEXT) {
                    _tstp->trace() << "TSTP::GDH_Security::update(): GDH SETUP LAST FOLLOW message received" << endl;
                    GDH_Setup_Last_Follow* message = buf->frame()->data<GDH_Setup_Last_Follow>();
                    //kout << "TSTP::GDH_Security::update(): GDH SETUP LAST FOLLOW message received with next = " << message->next() << ", " << message->finished() << " from " << author << endl;
                    List_Elements::Singly_Linked<Region::Space> *next = new List_Elements::Singly_Linked<Region::Space>(new Region::Space(message->next()));
                    _GDH_next.insert(next);
                    if(message->finished()) {
                        _GDH_state = GDH_WAITING_EXP;
                        //kout << "Last node changed to state GDH WAITING EXP";
                    }
                }
            } break;
            case GDH_ROUND: {
                GDH_Round * message = buf->frame()->data<GDH_Round>();
                Round_Key round_key = message->round_key();
                if(_tstp->here() != _tstp->sink()) {
                    _tstp->trace() << "TSTP::GDH_Security::update(): GDH ROUND message received" << endl;
                    //kout << "TSTP::GDH_Security::update(): GDH ROUND message received from " << author << endl;
                    switch(_GDH_node_type) {
                        case GDH_INTERMEDIATE: {
                            //calculate new partial key and send to next
                            round_key = _gdh.insert_key(round_key);
                            Buffer* resp = _tstp->alloc(sizeof(GDH_Round));
                            Region::Space* next = _GDH_next.head()->object();
                            new (resp->frame()) GDH_Round(message->group_id(), *next, round_key);
                            _tstp->marshal(resp);
                            _tstp->_nic->send(resp);
                            _GDH_state = GDH_WAITING_POP;
                        } break;
                        case GDH_LAST: {
                            //kout << "Last node received GDH_ROUND" << endl;
                            //we will want the old round key here
                            Round_Key my_key = _gdh.insert_key(round_key);
                            Buffer* resp = _tstp->alloc(sizeof(GDH_Response));
                            new (resp->frame()) GDH_Response(message->group_id(), TSTP::sink(), round_key);
                            _tstp->marshal(resp);
                            //kout << "Last node sent GDH_RESPONSE with round key(" << round_key << ") to the gateway" << endl;
                            _tstp->_nic->send(resp);
                            _GDH_state = GDH_WAITING_FINAL;
                            //send GDH_RESPONSE with round_key
                            for(auto next_el = _GDH_next.begin(); next_el != _GDH_next.end(); next_el++) {
                                resp = _tstp->alloc(sizeof(GDH_Broadcast));
                                Region::Space* next = next_el->object();
                                new (resp->frame()) GDH_Broadcast(message->group_id(), *next, my_key);
                                _tstp->marshal(resp);
                                _tstp->_nic->send(resp);
                                //kout << "Last node sent GDH_BROADCAST with my_key(" << my_key << ") to " << *next << endl;
                            }
                            //send GDH_BROADCAST with my_key
                        } break;
                        default: break;
                    }
                }
            } break;
            case GDH_BROADCAST: {
                //can be received from two nodes. Last and gateway
                GDH_Broadcast* message = buf->frame()->data<GDH_Broadcast>();
                _tstp->trace() << "TSTP::GDH_Security::update(): GDH BROADCAST message received" << endl;
                //kout << "TSTP::GDH_Security::update(): GDH BROADCAST message received from " << author << endl;
                if(_tstp->here() != _tstp->sink()) {
                    if(_GDH_state == GDH_WAITING_POP) {
                        Round_Key round_key = message->round_key();
                        round_key = _gdh.remove_key(round_key);
                        Buffer* resp = _tstp->alloc(sizeof(GDH_Response));
                        new (resp->frame()) GDH_Response(message->group_id(), TSTP::sink(), round_key);
                        _tstp->marshal(resp);
                        //kout << "Sending GDH Response with round key(" << round_key << ") to " << TSTP::sink() << endl;
                        _tstp->_nic->send(resp);
                        _GDH_state = GDH_WAITING_FINAL;
                    } else if(_GDH_state == GDH_WAITING_FINAL) {
                        Round_Key round_key = message->round_key();
                        round_key = _gdh.insert_key(round_key);
                        _GDH_key = round_key; //final key!
                        //kout << "We calculated the final key! key = " << _GDH_key << endl;
                    }
                } else { /*gateway*/
                    _GDH_key = _gdh.insert_key(message->round_key());
                    //kout << "Gateway calculated his final key! Key = " << _GDH_key << endl;
                }
            } break;
            case GDH_RESPONSE: {
                if(_tstp->here() == _tstp->sink()) {
                    _tstp->trace() << "TSTP::GDH_Security::update(): GDH RESPONSE message received from " << author << endl;
                    //kout << "TSTP::GDH_Security::update(): GDH RESPONSE message received from " << author << endl;
                    GDH_Response* message = buf->frame()->data<GDH_Response>();
                    Region::Space origin = buf->frame()->data<Header>()->origin(); //source of the message. Address
                    Round_Key round_key = message->round_key();
                    round_key = _gdh.insert_key(round_key);
                    Buffer* resp = _tstp->alloc(sizeof(GDH_Broadcast));
                    new (resp->frame()) GDH_Broadcast(message->group_id(), origin, round_key);
                    _tstp->marshal(resp);
                    //kout << "Sending GDH Broadcast with round key(" << round_key << ") to " << origin << endl;
                    _tstp->_nic->send(resp);
                }
            } break;
            default: break;
        }
    }
    if((!was_bootstrapped) && bootstrapped())
    _tstp->bootstrap();
}

void TSTP::Security::marshal(Buffer * buf)
{
    _tstp->trace() << "TSTP::Security::marshal(buf=" << buf << ")" << endl;
    buf->trusted = false;

    if(buf->frame()->data<Header>()->type() == TSTP::RESPONSE) {
        Peer * peer = 0;
        for(Peers::Element * el = _trusted_peers.head(); el; el = el->next())
            if(el->object()->valid_deploy(_tstp->destination(buf).center, _tstp->now())) {
                peer = el->object();
                break;
            }
        if(!peer)
            return;

        // Pad data to the size of the key
        unsigned int data_size = buf->size() - (sizeof(Response) - sizeof(Response::Data));
        Response * response = buf->frame()->data<Response>();
        unsigned char * data = response->data<unsigned char>();
        buf->size(sizeof(Response) - sizeof(Response::Data) + sizeof(Master_Secret));
        for(unsigned int i = data_size; i < sizeof(Master_Secret); i++)
            data[i] = 0;

        pack(data, peer);
        buf->trusted = true;
    }
}

TSTP::Security::~Security()
{
    _tstp->trace() << "TSTP::~Security()" << endl;
    while(auto el = _trusted_peers.remove_head())
        delete el->object();
    while(auto el = _pending_peers.remove_head())
        delete el->object();
    while(auto el = _pending_keys.remove_head())
        delete el->object();

//    _tstp->_nic->detach(this, 0);
}


// TSTP
// Class attributes


// Methods

void TSTP::update(NIC::Observed * obs, NIC::Protocol prot, Buffer * buf)
{
    trace() << "TSTP::update(obs=" << obs << ",buf=" << buf << ")" << endl;

    if(buf->is_microframe || !buf->destined_to_me || (_security_enabled && !buf->trusted))
        return;

    Packet * packet = buf->frame()->data<Packet>();

    if(packet->time() > now())
        return;

    switch(packet->type()) {
    case INTEREST: {
        Interest * interest = reinterpret_cast<Interest *>(packet);
        trace() << "TSTP::update:interest=" << interest << " => " << *interest << endl;
        // Check for local capability to respond and notify interested observers
        Responsives::List * list = _responsives[interest->unit()]; // TODO: What if sensor can answer multiple formats (e.g. int and float)
        if(list)
            for(Responsives::Element * el = list->head(); el; el = el->next()) {
                Responsive * responsive = el->object();
                if((now() < interest->region().t1) && interest->region().contains(responsive->origin(), interest->region().t1))
                    notify(responsive, buf);
            }
    } break;
    case RESPONSE: {
        Response * response = reinterpret_cast<Response *>(packet);
        trace() << "TSTP::update:response=" << response << " => " << *response << endl;
        trace() << "response->time() = " << response->time();
        trace() << "now() = " << now();
        if(response->time() < now()) {
            // Check region inclusion and notify interested observers
            Interests::List * list = _interested[response->unit()];
            if(list && !list->empty()) {
                for(Interests::Element * el = list->head(); el; el = el->next()) {
                    Interested * interested = el->object();
                    if(interested->region().contains(response->origin(), response->time()))
                        notify(interested, buf);
                }
            } else if(_sink)
                notify(0, buf);
        }
    } break;
    case COMMAND: {
        Command * command = reinterpret_cast<Command *>(packet);
        trace() << "TSTP::update:command=" << command << " => " << *command << endl;
        // Check for local capability to respond and notify interested observers
        Responsives::List * list = _responsives[command->unit()]; // TODO: What if sensor can answer multiple formats (e.g. int and float)
        if(list)
            for(Responsives::Element * el = list->head(); el; el = el->next()) {
                Responsive * responsive = el->object();
                if(command->region().contains(responsive->origin(), now()))
                    notify(responsive, buf);
            }
    } break;
    case CONTROL: {
        switch(buf->frame()->data<Control>()->subtype()) {
            case DH_REQUEST:
                trace() << "TSTP::update: DH_Request: " << *buf->frame()->data<DH_Request>() << endl;
                break;
            case DH_RESPONSE:
                trace() << "TSTP::update: DH_Response: " << *buf->frame()->data<DH_Response>() << endl;
                break;
            case AUTH_REQUEST:
                trace() << "TSTP::update: Auth_Request: " << *buf->frame()->data<Auth_Request>() << endl;
                break;
            case AUTH_GRANTED:
                trace() << "TSTP::update: Auth_Granted: " << *buf->frame()->data<Auth_Granted>() << endl;
                break;
            case REPORT: {
                trace() << "TSTP::update: Report: " << *buf->frame()->data<Report>() << endl;
                Report * report = reinterpret_cast<Report *>(packet);
                if(report->time() < now()) {
                    // Check region inclusion and advertise interested observers
                    Interests::List * list = _interested[report->unit()];
                    if(list)
                        for(Interests::Element * el = list->head(); el; el = el->next()) {
                            Interested * interested = el->object();
                            if(interested->region().contains(report->origin(), report->time()))
                                interested->advertise();
                        }
                    if(report->epoch_request() && (here() == sink())) {
                        trace() << "TSTP::update: responding to Epoch request" << endl;
                        Buffer * buf = alloc(sizeof(Epoch));
                        //Epoch * epoch = new (buf->frame()->data<Epoch>()) Epoch(Region(report->origin(), 0, now(), now() + Life_Keeper::PERIOD)); // TODO
                        marshal(buf);
                        //trace() << "TSTP::update:epoch = " << epoch << " => " << (*epoch) << endl;
                        _nic->send(buf);
                    }
                }
            } break;
            case KEEP_ALIVE:
                trace() << "TSTP::update: Keep_Alive: " << *buf->frame()->data<Keep_Alive>() << endl;
                break;
            case EPOCH: {
                trace() << "TSTP::update: Epoch: " << *buf->frame()->data<Epoch>() << endl;
                Epoch * epoch = reinterpret_cast<Epoch *>(packet);
                if(here() != sink()) {
                    _global_coordinates = epoch->coordinates();
                    _epoch = epoch->epoch();
                    trace() << "TSTP::update: Epoch: adjusted epoch Space-Time to: " << _global_coordinates << ", " << _epoch << endl;
                }
            } break;
            case GDH_SETUP_FIRST:
                trace() << "TSTP::update: GDH_Setup_First: " << *buf->frame()->data<GDH_Setup_First>() << endl;
                break;
            case GDH_SETUP_INTERMEDIATE:
                trace() << "TSTP::update: GDH_Setup_Intermediate: " << *buf->frame()->data<GDH_Setup_Intermediate>() << endl;
                break;
            case GDH_SETUP_LAST:
                trace() << "TSTP::update: GDH_Setup_Last: " << *buf->frame()->data<GDH_Setup_Last>() << endl;
                break;
            case GDH_SETUP_LAST_FOLLOW:
                trace() << "TSTP::update: GDH_Setup_Last_Follow: " << *buf->frame()->data<GDH_Setup_Last>() << endl;
                break;
            case GDH_ROUND:
                trace() << "TSTP::update: GDH_Round: " << *buf->frame()->data<GDH_Round>() << endl;
                break;
            case GDH_BROADCAST:
                trace() << "TSTP::update: GDH_Broadcast: " << *buf->frame()->data<GDH_Broadcast>() << endl;
                break;
            case GDH_RESPONSE:
                trace() << "TSTP::update: GDH_Response: " << *buf->frame()->data<GDH_Response>() << endl;
                break;
            default:
                trace() << "TSTP::update: Unrecognized Control subtype: " << buf->frame()->data<Control>()->subtype() << endl;
                break;
        }
    } break;
    default:
        trace() << "TSTP::update: Unrecognized packet type: " << packet->type() << endl;
        break;
    }
}
